import os
import torch
from torch.utils.data import Dataset

from libsvm.svmutil import *


class BreastcancerScale(Dataset):
    def __init__(self, directory, train=True) -> None:
        y, x = svm_read_problem(directory)
        # data preprocessing
        for i in range(len(y)):
            y[i] = 0 if y[i] == 2 else 1
        
        data = []
        for i in range(len(x)):
            data.append(list(x[i].values()))
        all_data = torch.tensor(data)
        all_targets = torch.tensor(y).type(torch.int64)

        self.train_size = 600  # 自訂訓練樣本數
        self.test_size = len(all_data) - self.train_size

        if train:
            self.data = all_data[:self.train_size]
            self.targets = all_targets[:self.train_size]
        else:
            self.data = all_data[self.train_size:]
            self.targets = all_targets[self.train_size:]

    def __len__(self):
        return self.data.shape[0]

    def __getitem__(self, idx):
        feature = self.data[idx]
        label = self.targets[idx]
        return feature, label
    

class DiabetesScale(Dataset):
    def __init__(self, directory, train=True) -> None:
        y, x = svm_read_problem(directory)
        # data preprocessing
        for i in range(len(y)):
            y[i] = 0 if y[i] == -1 else 1
        data_size = 8
        keys = [k for k in range(1, data_size+1)]
        for i in range(len(x)):
            for k in keys:
                if k not in x[i].keys():
                    x[i][k] = 0.0
        
        data = []
        for i in range(len(x)):
            data.append(list(x[i].values()))
        all_data = torch.tensor(data)
        all_targets = torch.tensor(y).type(torch.int64)

        self.train_size = 670  # 自訂訓練樣本數
        self.test_size = len(all_data) - self.train_size

        if train:
            self.data = all_data[:self.train_size]
            self.targets = all_targets[:self.train_size]
        else:
            self.data = all_data[self.train_size:]
            self.targets = all_targets[self.train_size:]

    def __len__(self):
        return self.data.shape[0]

    def __getitem__(self, idx):
        feature = self.data[idx]
        label = self.targets[idx]
        return feature, label


class HeartScale(Dataset):
    def __init__(self, directory, train=True) -> None:
        y, x = svm_read_problem(directory)
        # data preprocessing
        for i in range(len(y)):
            y[i] = 0 if y[i] == -1 else 1
        data_size = 13
        keys = [k for k in range(1, data_size+1)]
        for i in range(len(x)):
            for k in keys:
                if k not in x[i].keys():
                    x[i][k] = 0.0
        
        data = []
        for i in range(len(x)):
            data.append(list(x[i].values()))
        all_data = torch.tensor(data)
        all_targets = torch.tensor(y).type(torch.int64)

        self.train_size = 220  # 自訂訓練樣本數
        self.test_size = len(all_data) - self.train_size

        if train:
            self.data = all_data[:self.train_size]
            self.targets = all_targets[:self.train_size]
        else:
            self.data = all_data[self.train_size:]
            self.targets = all_targets[self.train_size:]

    def __len__(self):
        return self.data.shape[0]

    def __getitem__(self, idx):
        feature = self.data[idx]
        label = self.targets[idx]
        return feature, label