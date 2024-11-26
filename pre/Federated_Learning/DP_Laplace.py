import numpy as np
import torch
from utils.options import args_parser

args = args_parser()

def Laplace_Mechanism_Gradient(data, device):
    epsilon = pow(10, args.epsilon)

    maximum_list = []
    for k in data.keys():
        if 'weight' in k:
            for i in range(len(data[k])):
                maximum_list.append(max(data[k][i], key=abs))
        else:
            maximum_list.append(max(data[k], key=abs))
    sensitivity = abs(max(maximum_list, key=abs))
     
    noises = {}
    for name in data.keys():
        # add noises only on bias
        if 'bias' in name:
            noises[name] = torch.zeros(data[name].shape, dtype=torch.float32).to(device)
            for i in range(len(data[name])):
                noises[name][i] = np.random.laplace(0, sensitivity.cpu().numpy() / epsilon)
            noises[name] = torch.round(noises[name], decimals=args.precision)
        elif 'weight' in name:
            noises[name] = torch.zeros(data[name].shape, dtype=torch.float32).to(device)
            for i in range(len(data[name])):
                for j in range(len(data[name][i])):
                    noises[name][i][j] = np.random.laplace(0, sensitivity.cpu().numpy() / epsilon)
            noises[name] = torch.round(noises[name], decimals=args.precision)

    return noises

def Laplace_Mechanism_Bias(data, device):
    epsilon = pow(10, args.epsilon)

    maximum_list = []
    for k in data.keys():
        if 'weight' in k:
            for i in range(len(data[k])):
                maximum_list.append(max(data[k][i], key=abs))
        else:
            maximum_list.append(max(data[k], key=abs))
    sensitivity = abs(max(maximum_list, key=abs))
     
    noises = {}
    for name in data.keys():
        # add noises only on bias
        if 'bias' in name:
            noises[name] = torch.zeros(data[name].shape, dtype=torch.float32).to(device)
            for i in range(len(data[name])):
                noises[name][i] = np.random.laplace(0, sensitivity.cpu().numpy() / epsilon)
            noises[name] = torch.round(noises[name], decimals=args.precision)
        # elif 'weight' in name:
        #     noises[name] = torch.zeros(data[name].shape, dtype=torch.float32).to(device)
        #     for i in range(len(data[name])):
        #         for j in range(len(data[name][i])):
        #             noises[name][i][j] = np.random.laplace(0, sensitivity.cpu().numpy() / epsilon)
        #     noises[name] = torch.round(noises[name], decimals=args.precision)

    return noises

def Laplace_Mechanism_Lipschitz_Constant_Sensitivity(sensitivity, data, device):
    epsilon = pow(10, args.epsilon)
     
    noises = {}
    for name in data.keys():
        # add noises only on bias
        if 'bias' in name:
            noises[name] = torch.zeros(data[name].shape, dtype=torch.float32).to(device)
            for i in range(len(data[name])):
                noises[name][i] = np.random.laplace(0, sensitivity / epsilon)
            noises[name] = torch.round(noises[name], decimals=args.precision)
    return noises