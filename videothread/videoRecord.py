# -*- coding: utf-8 -*-
import cv2
import datetime
import os
import pandas as pd
import threading

from pysmx.SM2 import generate_keypair
from pysmx.SM2 import Sign
from PIL import Image


import tools
import SM3
import videoReadFrameb
import ctypes
import time


# 定义摄像头类
class cameraDevice(object):
    def __init__(self):
        # 获取摄像头对象，0 系统默认摄像头
        self.cap = cv2.VideoCapture(0, cv2.CAP_DSHOW)  # 20211210修改，解决摄像头释放时报错
        # 判断摄像头是否打开，没有则打开
        if not self.cap.isOpened():
            self.cap.open()

    def OtherSave(self):
        # 完成视频保存后继续计算剩余帧的hash
        global frame_id
        global filepath
        global len_para
        global frame_num

        cap = cv2.VideoCapture(filepath)  # import video files
        # 获取视频帧数CV_CAP_PROP_FRAME_COUNT
        frame_num = cap.get(7)
        print("Total frames: ", frame_num)
        # 根据录制到的frameId计算hash的起始id，例如21，31，41...
        new_frame_id = (frame_id - 50) // 10 * 10 + 1
        print("Record finished, start to hash frames from: ", new_frame_id)
        # # 创建一个空的Dataframe
        # video_hash_data = pd.DataFrame(columns=('frame_id', 'hash'))
        # 每10帧求一次hash
        while new_frame_id < frame_num // 10 * 10 + 1:
            # if (frame_num - new_frame_id) > 7:
            # if new_frame_id % 10 == 1:
            new_frame_hash = SM3.SM3B(videoReadFrameb.getFramebDirect(new_frame_id, new_frame_id + 9))
            sig = Sign(new_frame_hash, sk, '4a', len_para)
            new_hash_info = str(new_frame_id) + ' to ' + str(new_frame_id + 9) + ': ' + str(new_frame_hash)
            print(new_hash_info)
            # 数据写入DataFrame
            record_hash_data.loc[record_hash_data['frame_id'] == new_frame_id, ['hash']] = new_frame_hash
            record_hash_data.loc[record_hash_data['frame_id'] == new_frame_id, ['signature']] = sig
            new_frame_id += 10
            signature.append(sig)

        # 处理剩余不足10个的frame
        # if (frame_num - new_frame_id) < 8:
        new_frame_hash = SM3.SM3B(videoReadFrameb.getFramebDirect(new_frame_id, frame_num))
        new_sig = Sign(new_frame_hash, sk, '4a', len_para)
        new_hash_info = str(new_frame_id) + ' to ' + str(frame_num) + ': ' + str(new_frame_hash)
        print(new_hash_info)
        # 数据写入DataFrame
        record_hash_data.loc[record_hash_data['frame_id'] == new_frame_id, ['hash']] = new_frame_hash
        record_hash_data.loc[record_hash_data['frame_id'] == new_frame_id, ['signature']] = new_sig
        signature.append(new_sig)

        # 所有frame计算完毕后输出DataFrame到excel
        print(record_hash_data)
        record_hash_data.index.name = 'Index'
        record_hash_data.to_csv(r'data/record_hash.csv',  # 路径和文件名
                                # sheet_name='01',  # sheet 的名字
                                # float_format='%.2f',  # 保留两位小数
                                na_rep='null')  # 空值的显示

        print("All frame read!")
        return True

    def VideoCordFirst(self):

        global frame

        date_now = str(datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"))  # 获取的时间精确到秒
        # 用putText叠加水印(不能显示中文)
        font = cv2.FONT_HERSHEY_SIMPLEX
        frame = cv2.putText(frame, date_now, (10, 25), font, 1, (100, 100, 0), 2, 16)  # 第三个参数为文字左上角起始位置
        # 最后一个参数是LINE_AA
        # putText 各参数依次是图片，添加的文字，左上角坐标，字体，字体大小，颜色，字体粗细
        cv2.imshow('frame', frame)
        # return frame


class Thread1(threading.Thread):
    global camera01

    def __init__(self):
        super(Thread1, self).__init__()

    def saveVideoByHand(self):
        # Define the codec and create VideoWriter object
        # 视频编码
        global frame_id
        global frame
        image = Image.open('./pic/1.png').convert('L')
        histogram = image.histogram()  # 获取图像的直方图
        pixel_count = sum(histogram)  # 总像素数
        brightness = sum(index * value for index, value in enumerate(histogram)) / pixel_count  # 亮度计算公式
        contrast = sum(((index - brightness) ** 2) * value for index, value in enumerate(histogram)) / pixel_count
        brightness = str("%.2f" % brightness)
        contrast = str("%.2f" % contrast)
        date_now = str(datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"))
        font = cv2.FONT_HERSHEY_SIMPLEX
        src = cv2.imread('./qrcode.png')
        code = cv2.resize(src, (85, 83), interpolation=cv2.INTER_NEAREST)
        frame = cv2.putText(frame, date_now, (10, 470), font, 1, (0, 0, 0), 2, 16)
        frame = cv2.putText(frame, "0001", (550, 470), font, 1, (0, 0, 0), 2, 16)  # 第三个参数为文字左上角起始位置
        # 磁盘剩余空间/总空间
        frame = cv2.putText(frame, disk_info, (10, 440), font, 1, (0, 0, 0), 2, 16)
        # frame = cv2.putText(frame, brightness, (7, 25), font, 1, (0, 0, 0), 2, 16)
        frame = cv2.putText(frame, contrast, (7, 52), font, 1, (0, 0, 0), 2, 16)# 经纬度
        # 输出帧编号--调试用
        # frame_id += 1
        # print(frame_id)
        frame = cv2.putText(frame, str(frame_id), (570, 20), font, 1, (0, 0, 0), 2, 16)
        frame[392:475, 550:635] = code
        record_hash_data.loc[len(record_hash_data.index)] = ([frame_id, date_now, "", ""])

        cv2.imshow('frame', frame)
        # print("111111")1
        # frame1 = cv2.resize(frame, (720, 560), interpolation=cv2.INTER_LINEAR)
        out.write(frame.astype('uint8'))
        # out.write(frame.astype('uint8'))
        print(frame_id)
        # out.release()

        # 保存图片
        save_path = r'pic/{}.png'
        if not os.path.exists(save_path):
            os.makedirs(save_path)
        cv2.imwrite(save_path.format(frame_id), frame)

    def run(camera01):
        camera01.saveVideoByHand()
        # print("11111")


class Thread2(threading.Thread):
    global camera01
    global count

    def __init__(self):
        super(Thread2, self).__init__()

    def HashCode(self, count):
        global len_para
        frame_hash = SM3.SM3B(videoReadFrameb.getFrame(count - 60, count - 51))
        sig = Sign(frame_hash, sk, '4a', len_para)
        record_hash_data.loc[record_hash_data['frame_id'] == count - 60, ['hash']] = frame_hash
        record_hash_data.loc[record_hash_data['frame_id'] == count - 60, ['signature']] = sig
        signature.append(sig)

        # 输出DataFrame到excel
        record_hash_data.index.name = 'Index'
        record_hash_data.to_csv(r'data/record_hash.csv',  # 路径和文件名
                                # sheet_name='01',  # sheet 的名字
                                # float_format='%.2f',  # 保留两位小数
                                na_rep='null')  # 空值的显示

        # hash信息嵌入画面
        hash_info = str(count - 60) + ' to ' + str(count - 51) + ': ' + str(frame_hash)
        print(hash_info)
        # frame = cv2.putText(frame, hash_info, (0, 50), font, 0.3, (0, 0, 0), 1, 16)
        # cv2.imshow('frame', frame)
        # out1.write(frame.astype('uint8'))
        return True

    def run(camera01):
        global frame
        camera01.HashCode(count)
        # print("22222")

# 开始应用类
# 通过ip获取经纬度
if __name__ == "__main__":



    camera01 = cameraDevice()
    # camera02 = cameraDevice()
    c1 = str(camera01)
    camera_id = c1.split('0x')[1]
    camera_id = camera_id.strip('>')
    camera_id = camera_id[2:8]
    camera_id = int(camera_id, 16)
    pk, sk = generate_keypair(camera_id)
    len_para = 64

    fourcc = cv2.VideoWriter_fourcc(*'h264')
    width = 640
    height = 480

    filepath = 'output/output1.avi'
    out = cv2.VideoWriter(filepath, fourcc, 25.0, (width, height))
    signature = []
    frame_id = 0
    count = 51
    record_hash_data = pd.DataFrame(columns=('frame_id', 'time', 'hash', 'signature'))
    disk_info = tools.get_disk_space("/")
    start = time.time()

    while camera01.cap.isOpened:

        ret, frame = camera01.cap.read()
        if ret:
            camera01.VideoCordFirst()
            # print("1111")
            # frame, date_now = camera01.VideoCordFirst(frame)
            # time.sleep(20)
            if cv2.waitKey(1) & 0xFF == ord('s'):

                while camera01.cap.isOpened:
                    ret, frame = camera01.cap.read()
                    if ret:
                        frame_id += 1
                        t1 = Thread1()
                        t1.start()

                        if frame_id > 60 and frame_id % 10 == 1:
                            count += 10
                            t2 = Thread2()
                            t2.start()

                        if cv2.waitKey(1) & 0xFF == ord('q'):
                            # 视频暂停时输出数据
                            # print(record_hash_data)
                            break

                        # t1.join()
                    else:
                        break
                    t1.join()
                    # t2.join()
            if cv2.waitKey(1) & 0xFF == ord('q'):
                break
        else:
            break
    out.release()
    cv2.destroyAllWindows()
    camera01.cap.release()
    camera01.OtherSave()


    key_data = pd.DataFrame(columns=('pk', 'sk', 'camera_id', 'total'))
    key_data.loc[len(key_data.index)] = ([pk, sk, camera_id, frame_num])
    key_data.index.name = 'index'
    key_data.to_csv(r'data/key_data.csv', na_rep='null')

    signature_data = pd.DataFrame(data=signature)
    signature_data.to_csv(r'data/signature_data.csv', na_rep='null')
    dic = {'signature': signature}
    # verify_data = pd.DataFrame(dic)
    # verify_data.to_csv(r'data/verify_data.csv', index=False, na_rep='null')
    df = pd.read_csv('G:/pycharm/videothread1/data/record_hash.csv')
    sig_data = df['signature'].dropna(axis=0, how='all')
    verify_data = pd.DataFrame(sig_data)
    verify_data.to_csv(r'data/verify_data.csv', index=False, na_rep='null')

    # ffmpeg -i test.h264 -c:v copy test.avi
    # ffmpeg -i test.avi -c:v libx264 -preset ultrafast -crf 0 -pix_fmt yuv420p test.h264

    # 生成h264文件
    inputpath1 = "G:/pycharm/videothread1/output/output1.avi"
    outputpath1 = "G:/pycharm/videothread1/sei/sei.h264"
    cmd1 = "echo yes | ffmpeg -i " + inputpath1 + " -c:v libx264 -preset ultrafast -crf 0 -pix_fmt yuv420p " + outputpath1
    # cmd1 = "echo yes | ffmpeg -i "+inputpath1+" -vcodec h264 -s 640*480 -f m4v "+outputpath1
    os.system(cmd1)
    # time.sleep(10)

    # 插入签名
    ll = ctypes.cdll.LoadLibrary
    insei = ll("G:/pycharm/videothread1/sei/sei.dll")
    insei.insertSeiProcess()
    # time.sleep(10)
    end = time.time()
    print("运行时间：", end - start)

    # 生成带有签名的视频
    inputpath2 = "G:/pycharm/videothread1/sei/tmp.h264"
    outputpath2 = "G:/pycharm/videothread1/output/reliability1.avi"
    # cmd2 = "echo yes | ffmpeg -i "+inputpath2+" -c:v copy  "+outputpath2
    cmd2 = "echo yes | ffmpeg -i " + inputpath2 + " -vcodec h264 -s 640*480 " + outputpath2
    os.system(cmd2)


