# video
    Use python and C++ to generation and verification of trusted videos

# environment
    python 3.8.5 and opencv 4.6.0 
    install ffmpeg

# explanation
    VideoRecord.py is used to generate trusted videos
    Verify.py is used to verify a trusted video
    The output video folder is output, output1 is a recorded video, and relatibility1 is a trusted video
    The data folder is used to record hash value, Key Pair, ciphertext and frameid
    In the sei folder, sei.cpp is to write and read signature code in the code stream, sei.dll is the compiled file of sei.cpp, which can be called in python using c++functions
    
