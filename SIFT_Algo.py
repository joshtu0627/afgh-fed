import numpy as np
import cv2
import math
from matplotlib import pyplot as plt

sift = cv2.xfeatures2d.SIFT_create() 
FLANN_INDEX_KDTREE=0
indexParams=dict(algorithm=FLANN_INDEX_KDTREE,trees=5)
searchParams=dict(checks=50)
flann=cv2.FlannBasedMatcher(indexParams,searchParams)

def GetMatchNum(matches, ratio):
    '''返回特徵點匹配數量和matchesMask'''
    matches_mask = [[0,0] for i in range(len(matches))]
    match_num = 0
    for i, (m,n) in enumerate(matches):
        if m.distance < ratio * n.distance: #將距離比率小於ratio的匹配點篩選出來
            matches_mask[i] = [1,0]
            match_num += 1
    return match_num, matches_mask

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

    comparisonImageList = []
    for i in range(len(original_images)):
        keypoints_original, descriptors_original = sift.detectAndCompute(original_images[i], None)
        for j in range(len(reversed_images)):
            keypoints_reversed, descriptors_reversed = sift.detectAndCompute(reversed_images[j], None)
            if len(keypoints_original) == 0 or len(keypoints_reversed) == 0:
                continue
            
            matches = flann.knnMatch(descriptors_original, descriptors_reversed, k=2)
            match_num, matches_mask = GetMatchNum(matches, 0.9)
            score = match_num * 100 / len(matches)
            drawParams=dict(matchColor=(0,255,0),
                singlePointColor=(255,0,0),
                matchesMask=matches_mask,
                flags=0)
            comparisonImage=cv2.drawMatchesKnn(original_images[i],keypoints_original,reversed_images[j],keypoints_reversed,matches,None,**drawParams)
            comparisonImageList.append((comparisonImage,score)) #紀錄結果

    comparisonImageList.sort(key=lambda x:x[1],reverse=True) #按照匹配度排序
    count=len(comparisonImageList)
    if count == 0:
        continue
    column=3
    row=math.ceil(count/column)
    #圖片顯示
    figure,ax=plt.subplots(row,column)
    for index,(image,ratio) in enumerate(comparisonImageList):
        ax[int(index/column)][index%column].set_title('Similiarity %.2f%%' % ratio)
        ax[int(index/column)][index%column].imshow(image)
    plt.show()
            
            