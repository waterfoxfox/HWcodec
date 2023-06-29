# HWcodec
hardware codec for windows using ffmpeg (nvi qsv enc\dxva2 dec)
------- 
适用于windows平台的实时流硬件编码和解码，使用ffmpeg 3.4进行封装。<br> 注意：硬解码应该和渲染结合才能实现最低的CPU占用，本库因特殊要求，硬解码完后将图像数据从显存拷贝到了内存。<br><br>
## API接口：
请参考HwCodecSdk.h

更多资源见： https://mediapro.apifox.cn/

