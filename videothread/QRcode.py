import qrcode

qr = qrcode.QRCode()
qr.add_data('https://www.baidu.com')
img = qr.make_image()
img.show()
img.save('ts_qr.png')