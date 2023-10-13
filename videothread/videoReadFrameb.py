# 导入所需要的库
import cv2
import imageio
import os
import SM3
from PIL import Image

def getFrame(start_id, end_id, frame_b_ten=b""):
    if start_id > end_id:
        print("Error: end_id should not be small than start_id!")
    file_path = 'output/output1.avi'
    reader = imageio.get_reader(file_path, 'ffmpeg')
    video = []
    num = 0
    for r in reader:
        num = num + 1
        # print(frame)
        # print(frame.shape)
        video.append(r)
    i = 1
    while i < end_id + 1:
        framecut = video[start_id-1:end_id-1]
        # print("11111")
        if framecut != 0:
            if i > start_id - 1:
                # frame = video[i-1]
                img = Image.open('./pic/'+str(i)+'.png')
                frame = img.convert('P', palette=Image.ADAPTIVE, colors=256)
                frame_b = frame.tobytes()
                frame_b_ten += frame_b
                print("Frame ID read: ", i)

        else:
            print("Frame is empty!")
        i += 1
    # print("输出的10帧二进制流：", frame_b_ten)
    return frame_b_ten


# 从视频文件(直接获取特定帧)获取start_id到end_id帧的字节流并叠加
def getFramebDirect(start_id, end_id, frame_b_ten=b""):
    if start_id > end_id:
        print("Error: end_id should not be small than start_id!")
    file_path = 'output/output1.avi'
    cap = cv2.VideoCapture(file_path)  # import video files
    # # 获取视频帧数CV_CAP_PROP_FRAME_COUNT
    # frames_num = cap.get(7)
    # print("Total frame: ",frames_num)
    i = start_id
    while i < end_id + 1:
        ret, frame = cap.read()
        cap.set(cv2.CAP_PROP_POS_FRAMES, i - 1)
        if ret:
            img = Image.open('./pic/' + str(i) + '.png')
            frame = img.convert('P', palette=Image.ADAPTIVE, colors=256)
            # img = Image.open(frame)
            # frame = img.convert(' p', palette=Image.ADAPTIVE, colors=256)
            frame_b = frame.tobytes()
            frame_b_ten += frame_b
            print("Otherframe ID read:", i)

        else:
            print("Otherframe is empty!")
        i += 1
    # print("输出的10帧二进制流：", frame_b_ten)
    return frame_b_ten



