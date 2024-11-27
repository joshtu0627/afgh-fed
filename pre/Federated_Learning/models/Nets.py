#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Python version: 3.6

import torch
from torch import nn
import torch.nn.functional as F


class MLP(nn.Module):
    def __init__(self, dim_in, dim_hidden, dim_out):
        super(MLP, self).__init__()
        self.layer_input = nn.Linear(dim_in, dim_hidden)
        self.relu = nn.ReLU()
        self.dropout = nn.Dropout()
        self.layer_hidden = nn.Linear(dim_hidden, dim_out)

    def forward(self, x):
        x = x.view(-1, x.shape[1]*x.shape[-2]*x.shape[-1])
        x = self.layer_input(x)
        x = self.dropout(x)
        x = self.relu(x)
        x = self.layer_hidden(x)
        return x
    
    def spectral_norm(self, layer):
        u, s, v = torch.svd(layer.weight.data)
        return s.max().item()
    
    def lipschitz_constant(self):
        spectral_norms = [self.spectral_norm(layer) for layer in self.children() if isinstance(layer, nn.Linear)]
        loss_lipschitz_constant = 1  # CrossEntropyLoss 的 Lipschitz 常數為 1
        model_lipschitz_constant = loss_lipschitz_constant * torch.prod(torch.tensor(spectral_norms)).item()
        return model_lipschitz_constant