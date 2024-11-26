import numpy as np
import cv2
ROW = COL = 9

def ScoreRelative(original, reversed):
    original_hash = []
    reversed_hash = []
    for i in range(len(original)):
        image = cv2.resize(original[i],(ROW,COL),interpolation=cv2.INTER_CUBIC)
        original_hash.append(DifferenceHash(image))
    for i in range(len(reversed)):
        image = cv2.resize(reversed[i],(ROW,COL),interpolation=cv2.INTER_CUBIC)
        reversed_hash.append(DifferenceHash(image))

    # find most similar image between original and reversed
    score = []
    for i in range(len(original_hash)):
        dist = len(original_hash[i])
        best_reversed_idx = 0
        for j in range(len(reversed_hash)):
            curr_dist = HammingDistance(original_hash[i], reversed_hash[j])
            if curr_dist < dist:
                dist = curr_dist
                best_reversed_idx = j
        # score = [(index, score)]
        score.append((best_reversed_idx, (1-dist/len(original_hash[i]))*100))
    
    max_score = (0, 0)
    orig_idx = 0
    for i in range(len(score)):
        if score[i][1] > max_score[1]:
            max_score = score[i]
            orig_idx = i
    score.append(max_score)
    return score, orig_idx


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



for ep in range(-6, 7):
    epsilon = ep
    original_images = []
    reversed_images = []
    # read all the original images - batch size : 10
    for i in range(10):
        img = cv2.imread('./Experiment/bias/pictures/ep_'+str(epsilon)+'/original'+str(i)+'.jpg')
        img = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
        original_images.append(img)

    # read all the reversed images - layer_input->out_features : 200
    for i in range(200):
        img = cv2.imread('./Experiment/bias/pictures/ep_'+str(epsilon)+'/reversed'+str(i)+'.jpg')
        img = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
        reversed_images.append(img)

    score, orig_idx = ScoreRelative(original_images, reversed_images)

    original = cv2.imread('./Experiment/bias/pictures/ep_'+str(epsilon)+'/original'+str(orig_idx)+'.jpg')
    reversed = cv2.imread('./Experiment/bias/pictures/ep_'+str(epsilon)+'/reversed'+str(score[-1][0])+'.jpg')
    print('epsilon = ' + str(epsilon) + '\nbest score = ' + str(score[-1][1]))
    cv2.imshow('original', original)
    cv2.imshow('reversed', reversed)
    cv2.waitKey(0)
    print('---END---\n')
