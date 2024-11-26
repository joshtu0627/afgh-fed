import torch
import torchvision.transforms as transform
import cv2
from utils.options import args_parser

args = args_parser()
args.device = torch.device('cuda:{}'.format(args.gpu) if torch.cuda.is_available() and args.gpu != -1 else 'cpu')

def ReverseGradientWork(features, grad):
    specific_grad_weight = grad['layer_input.weight']
    specific_grad_bias = grad['layer_input.bias']
    reversed = []
    for neuron in range(len(specific_grad_weight)):
        weights = specific_grad_weight[neuron]
        bias = specific_grad_bias[neuron]
        if bias != 0:
            reversed.append(weights / bias)
        else:
            reversed.append(torch.zeros(weights.shape).to(args.device))

    reversed_grad = []
    for i in range(len(features)):
        original_data = torch.flatten(features[i]).to(args.device)
        reversed_best = reversed[0]
        for j in range(len(reversed)):
            if torch.sum(original_data == reversed[j]) > torch.sum(original_data == reversed_best):
                reversed_best = reversed[j]
        # accuracy = torch.sum(torch.round(reversed_best, decimals=3) == torch.round(original_data, decimals=3)) / original_data.shape[0] * 100
        reversed_grad.append(reversed_best)
        
    return reversed_grad


def ReverseGradientWork_Local(features, grad):
    specific_grad_weight = grad['layer_input.weight']
    specific_grad_bias = grad['layer_input.bias']
    reversed = []
    for neuron in range(len(specific_grad_weight)):
        weights = specific_grad_weight[neuron]
        bias = specific_grad_bias[neuron]
        if bias != 0:
            reversed.append(weights / bias)
        else:
            reversed.append(torch.zeros(weights.shape).to(args.device))

    # 直接由feature和reversed參數來評判兩者相似程度並不標準，需要先轉為圖片再來判斷相似程度
    for i in range(len(features)):
        img = transform.ToPILImage()(features[i].reshape(1, 28, 28))
        img.save('./Experiment/weight&bias/pictures/ep_'+str(args.epsilon)+'/original'+str(i)+'.jpg')
    for i in range(len(reversed)):
        img = transform.ToPILImage()(reversed[i].reshape(1, 28, 28))
        img.save('./Experiment/weight&bias/pictures/ep_'+str(args.epsilon)+'/reversed'+str(i)+'.jpg')
    # score = ScoreRelative(features, reversed)
    # score_file = open('./Experiment/bias/HashAlgo_ep'+str(args.epsilon)+'.txt', 'w')
    # for i in range(len(score)):
    #     score_file.write(str(round(score[i], args.precision))+'\n')
    # score_file.close()

    return


def ScoreRelative(features, reversed):
    features_hash = []
    reversed_hash = []
    for i in range(len(features)):
        image = cv2.resize(features[i][0].cpu().numpy(),(5,5),interpolation=cv2.INTER_CUBIC)
        features_hash.append(DifferenceHash(image))
    for i in range(len(reversed)):
        image = cv2.resize(reversed[i].cpu().numpy(),(5,5),interpolation=cv2.INTER_CUBIC)
        reversed_hash.append(DifferenceHash(image))

    # find most similar image between features and reversed
    score = []
    for i in range(len(features_hash)):
        dist = len(features_hash[i])
        idx = 0
        for j in range(len(reversed_hash)):
            curr_dist = HammingDistance(features_hash[i], reversed_hash[j])
            if curr_dist < dist:
                dist = curr_dist
                idx = j
        score.append((1-dist/len(features_hash[i]))*100)
        img = transform.ToPILImage()(features[i].reshape(1, 28, 28))
        img.save('./Experiment/pictures/ep_'+str(args.epsilon)+'/original'+str(i)+'.jpg')
        img = transform.ToPILImage()(reversed[idx].reshape(1, 28, 28))
        img.save('./Experiment/pictures/ep_'+str(args.epsilon)+'/reversed'+str(i)+'.jpg')
        
    
    max_score = 0
    for i in range(len(score)):
        if score[i] > max_score:
            max_score = score[i]
    score.append(max_score)
    return score


def AverageHash(image):
    total = 0
    for i in range(image.shape[0]):
        for j in range(image.shape[1]-1):
            total += image[i][j]
    average = total / (image.shape[0]*image.shape[1])

    avg_hash = ''
    for i in range(image.shape[0]):
        for j in range(image.shape[1]-1):
            if image[i][j] > average:
                avg_hash += '1'
            else:
                avg_hash += '0'
    return avg_hash


def DifferenceHash(image):
    diff_hash = ''
    for i in range(image.shape[0]):
        for j in range(image.shape[1]-1):
            if image[i, j] > image[i, j+1]:
                diff_hash += '1'
            else:
                diff_hash += '0'
    return diff_hash


def HammingDistance(hash_x, hash_y):
    if len(hash_x) != len(hash_y):
        return -1
    
    dist = 0
    for i in range(len(hash_x)):
        if hash_x[i] != hash_y[i]:
            dist += 1
    return dist