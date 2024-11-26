#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Python version: 3.6

import torch
from torch import nn, autograd
from torch.utils.data import DataLoader, Dataset
import numpy as np
from sklearn import metrics
import MVG
import LocalDP
import read_write
import matplotlib.pyplot as plt
import matplotlib
matplotlib.use('TkAgg')

from gradient_reverse import ReverseGradientWork_Local
from collections import defaultdict

PRECISION = 4

class DatasetSplit(Dataset):
    def __init__(self, dataset, idxs):
        self.dataset = dataset
        self.idxs = list(idxs)

    def __len__(self):
        return len(self.idxs)

    def __getitem__(self, item):
        feature, label = self.dataset[self.idxs[item]]
        return feature, label


class LocalUpdate(object):
    def __init__(self, args, dataset=None, idxs=None, client=-1):
        self.args = args
        self.loss_func = nn.CrossEntropyLoss()
        self.selected_client = client
        self.ldr_train = DataLoader(DatasetSplit(dataset, idxs), batch_size=self.args.local_bs, shuffle=True)

    def train(self, net):
        net.train()
        # train and update
        optimizer = torch.optim.SGD(
            net.parameters(), lr=self.args.lr, momentum=self.args.momentum)

        # Initial gradients
        grad = {}
        for name, param in net.named_parameters():
            grad[name] = torch.zeros(param.shape, dtype=torch.float32).to(self.args.device)

        # reversed_grad = []
        curr_grad = {}
        epoch_loss = []
        for eps in range(10, 11):
            epsilon = pow(10, eps)
            # origin_file = open('./Dataset/'+self.args.dataset+'/Row2/Round2/'+self.args.dataset+'_epsilon-'+str(eps)+'_origin.txt', 'w')
            # reverse_file = open('./Dataset/'+self.args.dataset+'/Row2/Round2/'+self.args.dataset+'_epsilon-'+str(eps)+'_reverse.txt', 'w')
            # gradient_file = open('./Dataset/'+self.args.dataset+'/Row1/'+self.args.dataset+'_epsilon-'+str(eps)+'_gradient.txt', 'w')
            # perturb_file = open('./Dataset/'+self.args.dataset+'/Row1/'+self.args.dataset+'_epsilon-'+str(eps)+'_perturb.txt', 'w')
            for iter in range(self.args.local_ep):
                batch_loss = []
                for batch_idx, (features, labels) in enumerate(self.ldr_train):
                    features, labels = features.to(self.args.device), labels.to(self.args.device)
                    net.zero_grad()
                    log_probs = net(features)
                    loss = self.loss_func(log_probs, labels)
                    loss.backward()
                    optimizer.step()
                    for name, param in net.named_parameters():
                        grad[name] += param.grad
                        # Save current gardients
                        curr_grad[name] = param.grad


                    # perturb = {}
                    # noises = MVG.generate_mvg(curr_grad, epsilon, self.args.device)
                    # for name in curr_grad.keys():
                    #     perturb[name] = curr_grad[name] + noises[name]

                    # # Reversing Gradients
                    # reversed_grad = ReverseGradientWork_Local(features, perturb)
                    
                    # for i in range(len(features)):
                    #     for data in features[i]:
                    #         origin_file.write(str(round(data.item(), PRECISION))+' ')
                    #     for data in reversed_grad[i]:
                    #         reverse_file.write(str(round(data.item(), PRECISION))+' ')
                    
                    # for name in curr_grad.keys():
                    #     if 'weight' in name:
                    #         for i in range(len(curr_grad[name])):
                    #             for j in range(len(curr_grad[name][i])):
                    #                 gradient_file.write(str(round(curr_grad[name][i][j].item(), PRECISION))+' ')
                    #                 perturb_file.write(str(round(perturb[name][i][j].item(), PRECISION))+' ')
                    #     elif 'bias' in name:
                    #         for i in range(len(curr_grad[name])):
                    #             gradient_file.write(str(round(curr_grad[name][i].item(), PRECISION))+' ')
                    #             perturb_file.write(str(round(perturb[name][i].item(), PRECISION))+' ')


                    if self.args.verbose and batch_idx % 10 == 0:
                        print('Update Epoch: {} [{}/{} ({:.0f}%)]\tLoss: {:.6f}'.format(iter, batch_idx * len(features), len(self.ldr_train.dataset), 100. * batch_idx / len(self.ldr_train), loss.item()))
                    batch_loss.append(loss.item())
                epoch_loss.append(sum(batch_loss)/len(batch_loss))

                # origin_file.close()
                # reverse_file.close()
                # gradient_file.close()
                # perturb_file.close()

        # calculate noises
        noises = MVG.generate_mvg(grad, epsilon, self.args.device)

        # calculate grad + noises
        grad_perturb = {}
        for name in grad.keys():
            grad_perturb[name] = grad[name] + noises[name]

        # write noises to be encrypted
        read_write.WriteNoiseFile('./pre/clients_noises/client' + str(self.selected_client) + '.txt', noises)

        return net.state_dict(), sum(epoch_loss) / len(epoch_loss), grad, grad_perturb, noises
