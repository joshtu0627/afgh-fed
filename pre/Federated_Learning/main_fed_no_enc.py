#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Python version: 3.6

import read_write
import data.LIBSVM as lib
from libsvm.svmutil import *
from models.test import test
from models.Fed import FedAvg, FedAvg_RemovePerturbation
from models.Nets import MLP
from models.Update_Bias import LocalUpdate
from utils.options import args_parser
from utils.sampling import breastcancer_iid, diabetes_iid, heart_iid, mnist_iid, mnist_noniid
import time
import torch
from torchvision import datasets, transforms
import numpy as np
import copy
import os
import matplotlib.pyplot as plt
import matplotlib
# matplotlib.use('Agg')

if __name__ == '__main__':
    print(torch.__version__)
    # counting execution time
    start_time = time.time()

    # parse args
    args = args_parser()
    args.device = torch.device('cuda:{}'.format(
        args.gpu) if torch.cuda.is_available() and args.gpu != -1 else 'cpu')
    print('Using Device : ', args.device)

    # load dataset and split users
    if args.dataset == 'breastcancer':
        dataset_train = lib.BreastcancerScale('./pre/FL_RAPPOR_v1.0/data/BreastcancerScale.txt', train=True)
        dataset_test = lib.BreastcancerScale('./pre/FL_RAPPOR_v1.0/data/BreastcancerScale.txt', train=False)

        # sample users
        dict_users = breastcancer_iid(dataset_train.train_size, args.num_users)

        # build model
        if args.model == 'mlp':
            net_glob = MLP(dim_in=10, dim_hidden=5, dim_out=args.num_classes).to(args.device)
        else:
            exit('Error: unrecognized model')
    elif args.dataset == 'diabetes':
        dataset_train = lib.DiabetesScale('./pre/FL_RAPPOR_v1.0/data/DiabetesScale.txt', train=True)
        dataset_test = lib.DiabetesScale('./pre/FL_RAPPOR_v1.0/data/DiabetesScale.txt', train=False)

        # sample users
        dict_users = diabetes_iid(dataset_train.train_size, args.num_users)

        # build model
        if args.model == 'mlp':
            net_glob = MLP(dim_in=8, dim_hidden=5, dim_out=args.num_classes).to(args.device)
        else:
            exit('Error: unrecognized model')
    elif args.dataset == 'heart':
        dataset_train = lib.HeartScale('./pre/FL_RAPPOR_v1.0/data/HeartScale.txt', train=True)
        dataset_test = lib.HeartScale('./pre/FL_RAPPOR_v1.0/data/HeartScale.txt', train=False)

        # sample users
        dict_users = heart_iid(dataset_train.train_size, args.num_users)

        # build model
        if args.model == 'mlp':
            net_glob = MLP(dim_in=13, dim_hidden=5, dim_out=args.num_classes).to(args.device)
        else:
            exit('Error: unrecognized model')
    elif args.dataset == 'mnist':
        trans_mnist = transforms.Compose([transforms.ToTensor(), transforms.Normalize((0.1307,), (0.3081,))])
        dataset_train = datasets.MNIST('../data/mnist/', train=True, download=True, transform=trans_mnist)
        dataset_test = datasets.MNIST('../data/mnist/', train=False, download=True, transform=trans_mnist)

        # sample users
        dict_users = mnist_iid(dataset_train, args.num_users)

        # build model
        img_size = dataset_train[0][0].shape
        if args.model == 'mlp':
            len_in = 1
            for x in img_size:
                len_in *= x
            net_glob = MLP(dim_in=len_in, dim_hidden=200,
                        dim_out=args.num_classes).to(args.device)
        else:
            exit('Error: unrecognized model')
    else:
        exit('Error: unrecognized dataset')
    
    print(net_glob)
    net_glob.train()

    # copy weights
    w_glob = net_glob.state_dict()

    # training
    loss_train = []
    cv_loss, cv_acc = [], []
    val_loss_pre, counter = 0, 0
    net_best = None
    best_loss = None
    val_acc_list, net_list = [], []

    if args.all_clients:
        print("Aggregation over all clients")
        w_locals = [w_glob for i in range(args.num_users)]
    for iter in range(args.epochs):
        loss_locals = []
        if not args.all_clients:
            w_locals = []
            w_origin = []
        m = max(int(args.frac * args.num_users), 1)
        # idxs_users = np.random.choice(range(args.num_users), m, replace=False)
        idxs_users = np.array(range(m))
        for idx in idxs_users:
            local = LocalUpdate(args=args, dataset=dataset_train,idxs=dict_users[idx], client=idx)
            w, loss, grad, grad_perturb, noises = local.train(net=copy.deepcopy(net_glob).to(args.device))

            if args.all_clients:
                w_locals[idx] = copy.deepcopy(grad_perturb)
            else:
                w_locals.append(copy.deepcopy(grad_perturb))
                w_origin.append(copy.deepcopy(grad))
            loss_locals.append(copy.deepcopy(loss))

        # Server read sum of noises
        # os.system('./pre/bin/relic_hom')
        # noises_sum = read_write.ReadNoisesSumFile('./pre/clients_noises/noises_sum.txt')

        # update global weights
        # w_glob = FedAvg_RemovePerturbation(w_locals, noises_sum)
        w_glob = FedAvg(w_locals)
        # w_gl0b = FedAvg(w_origin)
        # w_glob = FedAvg(w_locals)

        # add Gradients to parameters
        for name, params in net_glob.state_dict().items():
            params.add_(-1 * w_glob[name] * args.lr)

        # print loss
        loss_avg = sum(loss_locals) / len(loss_locals)
        print('Round {:3d}, Average loss {:.3f}'.format(iter, loss_avg))
        loss_train.append(loss_avg)

    # plot loss curve
    plt.figure()
    plt.plot(range(len(loss_train)), loss_train)
    plt.ylabel('train_loss')
    # plt.savefig('./pre/Federated_Learning/save/fed_{}_{}_{}_C{}_iid{}.png'.format(args.dataset, args.model, args.epochs, args.frac, args.iid))

    # testing
    net_glob.eval()
    acc_train, loss_train = test(net_glob, dataset_train, args)
    acc_test, loss_test = test(net_glob, dataset_test, args)
    print("Training accuracy: {:.2f}".format(acc_train))
    print("Testing accuracy: {:.2f}".format(acc_test))
    print('Execution time: {:.1f} seconds'.format(time.time() - start_time))
    # origin_file = open('./Dataset/'+args.dataset+'/Row1/'+args.dataset+'_origin_accuracy.txt', 'w')
    # origin_file.write(str(round(acc_test.item(), args.precision))+'\n')
    # origin_file.close()
    # noises_file = open('./Accuracy/Row1/noises_accuracy.txt', 'a')
    # noises_file.write(str(round(acc_test.item(), args.precision))+'\n')
    # noises_file.close()
    print('Task Completed')
