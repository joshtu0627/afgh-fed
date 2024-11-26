#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Python version: 3.6

import torch
from torch import nn, autograd
from torch.utils.data import DataLoader, Dataset
import numpy as np
import DP_Laplace
import DP_Gaussian
import read_write
from utils.options import args_parser
from gradient_reverse import ReverseGradientWork_Local

args = args_parser()

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

    def train_with_bias_noise(self, net):
        net.train()
        # train and update
        optimizer = torch.optim.SGD(
            net.parameters(), lr=self.args.lr, momentum=self.args.momentum)

        # Initial gradients
        grad = {}
        for name, param in net.named_parameters():
            grad[name] = torch.zeros(param.shape, dtype=torch.float32).to(self.args.device)

        epoch_loss = []
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

                if self.args.verbose and batch_idx % 10 == 0:
                    print('Update Epoch: {} [{}/{} ({:.0f}%)]\tLoss: {:.6f}'.format(iter, batch_idx * len(features), len(self.ldr_train.dataset), 100. * batch_idx / len(self.ldr_train), loss.item()))
                batch_loss.append(loss.item())
            epoch_loss.append(sum(batch_loss)/len(batch_loss))

        # calculate noises
        # noises = DP_Gaussian.Gaussian_Mechanism(grad, self.args.device)
        noises = DP_Laplace.Laplace_Mechanism_Bias(grad, self.args.device)

        # calculate grad + noises       
        grad_perturb = {}
        for name in grad.keys():
            # add noises only on bias
            # grad_perturb[name] = grad[name] + noises[name]
            if 'bias' in name:
                grad_perturb[name] = grad[name] + noises[name]
            else:
                grad_perturb[name] = grad[name]

        # write noises to be encrypted
        # read_write.WriteNoiseFile('./pre/clients_noises/client' + str(self.selected_client) + '.txt', noises)

        return net.state_dict(), sum(epoch_loss) / len(epoch_loss), grad, grad_perturb, noises

    def train_with_gradient_noise(self, net):
        net.train()
        # train and update
        optimizer = torch.optim.SGD(
            net.parameters(), lr=self.args.lr, momentum=self.args.momentum)

        # Initial gradients
        grad = {}
        for name, param in net.named_parameters():
            grad[name] = torch.zeros(param.shape, dtype=torch.float32).to(self.args.device)

        epoch_loss = []
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

                if self.args.verbose and batch_idx % 10 == 0:
                    print('Update Epoch: {} [{}/{} ({:.0f}%)]\tLoss: {:.6f}'.format(iter, batch_idx * len(features), len(self.ldr_train.dataset), 100. * batch_idx / len(self.ldr_train), loss.item()))
                batch_loss.append(loss.item())
            epoch_loss.append(sum(batch_loss)/len(batch_loss))

        # calculate noises
        # noises = DP_Gaussian.Gaussian_Mechanism(grad, self.args.device)
        noises = DP_Laplace.Laplace_Mechanism_Gradient(grad, self.args.device)

        # calculate grad + noises       
        grad_perturb = {}
        for name in grad.keys():
            # add noises on whole gradient
            grad_perturb[name] = grad[name] + noises[name]

        # write noises to be encrypted
        # read_write.WriteNoiseFile('./pre/clients_noises/client' + str(self.selected_client) + '.txt', noises)

        return net.state_dict(), sum(epoch_loss) / len(epoch_loss), grad, grad_perturb, noises




    def train_no_noise(self, net):
        net.train()
        # train and update
        optimizer = torch.optim.SGD(
            net.parameters(), lr=self.args.lr, momentum=self.args.momentum)

        # Initial gradients
        grad = {}
        for name, param in net.named_parameters():
            grad[name] = torch.zeros(param.shape, dtype=torch.float32).to(self.args.device)

        epoch_loss = []
        total_correct = 0  # 計算正確預測的總數
        total_samples = 0  # 計算總樣本數

        for iter in range(self.args.local_ep):
            batch_loss = []
            for batch_idx, (features, labels) in enumerate(self.ldr_train):
                features, labels = features.to(self.args.device), labels.to(self.args.device)
                net.zero_grad()
                log_probs = net(features)
                loss = self.loss_func(log_probs, labels)
                loss.backward()
                optimizer.step()

                # 更新梯度
                for name, param in net.named_parameters():
                    grad[name] += param.grad

                # 計算準確率
                _, predicted = torch.max(log_probs, 1)
                total_correct += (predicted == labels).sum().item()  # 計算當前批次正確的預測數
                total_samples += labels.size(0)  # 累積樣本數

                if self.args.verbose and batch_idx % 10 == 0:
                    print('Update Epoch: {} [{}/{} ({:.0f}%)]\tLoss: {:.6f}'.format(
                        iter, batch_idx * len(features), len(self.ldr_train.dataset),
                        100. * batch_idx / len(self.ldr_train), loss.item()))
                batch_loss.append(loss.item())
            
            epoch_loss.append(sum(batch_loss) / len(batch_loss))

        # 計算最終準確率
        accuracy = 100.0 * total_correct / total_samples
        print(f'Client {self.selected_client} - Epoch {iter+1} Accuracy: {accuracy:.2f}%')

        return net.state_dict(), sum(epoch_loss) / len(epoch_loss), grad