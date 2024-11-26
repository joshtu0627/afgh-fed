#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Python version: 3.6

import copy
import torch
from torch import nn


def FedAvg(w):
    w_avg = copy.deepcopy(w[0])
    for k in w_avg.keys():
        for i in range(1, len(w)):
            w_avg[k] += w[i][k]
        w_avg[k] = torch.div(w_avg[k], len(w))
    return w_avg


def FedAvg_RemovePerturbation(w, n_sum):
    w_avg = copy.deepcopy(w[0])
    client_no = len(w)
    n_count = 0
    for key in w_avg.keys():
        for client in range(1, client_no):
            w_avg[key] += w[client][key]

        # only bias' needed to remove noises
        if 'bias' in key:
            for i in range(len(w_avg[key])):
                w_avg[key][i] -= n_sum[n_count]
                n_count += 1
        w_avg[key] = torch.div(w_avg[key], client_no)
    return w_avg
