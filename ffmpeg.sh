ffmpeg -i ../badapple.webm -vf "fps=15,scale=128:64,format=monob" -f rawvideo badapple.bin
