# send a file using nvidia hardware  h265 encoding to an rtp stream. -re to enable real time mode
ffmpeg -hwaccel cuda -hwaccel_output_format cuda  -re  -i test_media/ISS_View_of_Planet_Earth_2160p.mp4 \
                -c:a copy -c:v hevc_nvenc -f rtp "rtp://127.0.0.1:50574"
#receive that file using ffplay -> copy the SDP text from the output of the above command into a file called file_sdp.sdp then run
ffplay file_sdp.sdp  -protocol_whitelist file,udp,rtp

# send webcam using nvidia hardware h265 encoding to an rtp stream
 ffmpeg -hwaccel cuda -hwaccel_output_format cuda  \
    -f v4l2 -framerate 30 -video_size 1280x720 -i /dev/video0 \
    -c:v hevc_nvenc -preset p1 -tune ull  -muxpreload 0 -muxdelay 0 -rc cbr -cbr true \
    -f rtp "rtp://127.0.0.1:60000"


##eceive that stream using ffplay -> copy the SDP text from the output of the above command into a file called webcame_sdp.sdp then run
ffplay webcam_sdp.sdp -protocol_whitelist file,udp,rtp



#  get all supported formats
v4l2-ctl --list-formats-ext 

#nvidia codec options
https://gist.github.com/nico-lab/c2d192cbb793dfd241c1eafeb52a21c3

# for psnr and ssim ensure that the videos are same framerate and scaled correctly

# psnr
ffmpeg \
    -i transmitted.mp4 \
    -i original.mp4 \
    psnr="psnr.log" \ 
    -f null -

# ssim
ffmpeg \
    -i transmitted.mp4 \
    -i original.mp4 \
    ssim="ssim.log" \ 
    -f null -

# get last 60 seconds of  file into new file
ffmpeg -sseof -60 -i A012C002H2201038S_CANON_01-Surface_Tension.MXF -c copy tesntion_out.MKV            
ffmpeg -ss 00:04:18 -i A012C002H2201038S_CANON_01-Surface_Tension.MXF -t 60 -c copy trimmed.MKV


# ffmpeg -y -hwaccel cuda -hwaccel_output_format cuda  \
# -r 60000/1001  -i water_bubble.mp4 -map 0 \
# -c:v h264_nvenc -rc cbr   -no-scenecut true  -vf format=yuv420p -c:a copy  water_bubbles_h264_cbr_hq.mp4
ffmpeg -y -i water_bubble.mp4 -map 0:v -c:v libx264 -crf 18 -c:a copy water_bubbles_h264_cbr_hq.mp4

# h265 to h264 constant bit rate 10 bit
ffmpeg -y -r 60000/1001  -threads 0 -i water_bubble.mp4 -map 0:v -c:v libx264 -no-scenecut true -g 60  -crf 18 -c:a copy water_bubbles_h264_cbr_hq.mp4

# h265 to h254 variable bit rate 10 bit twoo pass
ffmpeg -y -r 60000/1001  -threads 0 -i water_bubble.mp4 -c:v libx264 -b:v 110M -maxrate 130.0M -bufsize 100M -pass 1 -f mp4 /dev/null && \
ffmpeg -y -r 60000/1001  -threads 0 -i water_bubble.mp4 -c:v libx264 -b:v 110M -maxrate 130.0M -bufsize 100M -pass 2 water_bubble_h264_vbr.mp4


ffmpeg h264_nvenc -h


Encoder h264_nvenc [NVIDIA NVENC H.264 encoder]:
    General capabilities: dr1 delay hardware 
    Threading capabilities: none
    Supported hardware devices: cuda cuda 
    Supported pixel formats: yuv420p nv12 p010le yuv444p p016le yuv444p16le bgr0 bgra rgb0 rgba x2rgb10le x2bgr10le gbrp gbrp16le cuda
h264_nvenc AVOptions:
  -preset            <int>        E..V....... Set the encoding preset (from 0 to 18) (default p4)
     default         0            E..V....... 
     slow            1            E..V....... hq 2 passes
     medium          2            E..V....... hq 1 pass
     fast            3            E..V....... hp 1 pass
     hp              4            E..V....... 
     hq              5            E..V....... 
     bd              6            E..V....... 
     ll              7            E..V....... low latency
     llhq            8            E..V....... low latency hq
     llhp            9            E..V....... low latency hp
     lossless        10           E..V....... 
     losslesshp      11           E..V....... 
     p1              12           E..V....... fastest (lowest quality)
     p2              13           E..V....... faster (lower quality)
     p3              14           E..V....... fast (low quality)
     p4              15           E..V....... medium (default)
     p5              16           E..V....... slow (good quality)
     p6              17           E..V....... slower (better quality)
     p7              18           E..V....... slowest (best quality)
  -tune              <int>        E..V....... Set the encoding tuning info (from 1 to 4) (default hq)
     hq              1            E..V....... High quality
     ll              2            E..V....... Low latency
     ull             3            E..V....... Ultra low latency
     lossless        4            E..V....... Lossless
  -profile           <int>        E..V....... Set the encoding profile (from 0 to 3) (default main)
     baseline        0            E..V....... 
     main            1            E..V....... 
     high            2            E..V....... 
     high444p        3            E..V....... 
  -level             <int>        E..V....... Set the encoding level restriction (from 0 to 62) (default auto)
     auto            0            E..V....... 
     1               10           E..V....... 
     1.0             10           E..V....... 
     1b              9            E..V....... 
     1.0b            9            E..V....... 
     1.1             11           E..V....... 
     1.2             12           E..V....... 
     1.3             13           E..V....... 
     2               20           E..V....... 
     2.0             20           E..V....... 
     2.1             21           E..V....... 
     2.2             22           E..V....... 
     3               30           E..V....... 
     3.0             30           E..V....... 
     3.1             31           E..V....... 
     3.2             32           E..V....... 
     4               40           E..V....... 
     4.0             40           E..V....... 
     4.1             41           E..V....... 
     4.2             42           E..V....... 
     5               50           E..V....... 
     5.0             50           E..V....... 
     5.1             51           E..V....... 
     5.2             52           E..V....... 
     6.0             60           E..V....... 
     6.1             61           E..V....... 
     6.2             62           E..V....... 
  -rc                <int>        E..V....... Override the preset rate-control (from -1 to INT_MAX) (default -1)
     constqp         0            E..V....... Constant QP mode
     vbr             1            E..V....... Variable bitrate mode
     cbr             2            E..V....... Constant bitrate mode
     vbr_minqp       8388612      E..V....... Variable bitrate mode with MinQP (deprecated)
     ll_2pass_quality 8388616      E..V....... Multi-pass optimized for image quality (deprecated)
     ll_2pass_size   8388624      E..V....... Multi-pass optimized for constant frame size (deprecated)
     vbr_2pass       8388640      E..V....... Multi-pass variable bitrate mode (deprecated)
     cbr_ld_hq       8388616      E..V....... Constant bitrate low delay high quality mode
     cbr_hq          8388624      E..V....... Constant bitrate high quality mode
     vbr_hq          8388640      E..V....... Variable bitrate high quality mode
  -rc-lookahead      <int>        E..V....... Number of frames to look ahead for rate-control (from 0 to INT_MAX) (default 0)
  -surfaces          <int>        E..V....... Number of concurrent surfaces (from 0 to 64) (default 0)
  -cbr               <boolean>    E..V....... Use cbr encoding mode (default false)
  -2pass             <boolean>    E..V....... Use 2pass encoding mode (default auto)
  -gpu               <int>        E..V....... Selects which NVENC capable GPU to use. First GPU is 0, second is 1, and so on. (from -2 to INT_MAX) (default any)
     any             -1           E..V....... Pick the first device available
     list            -2           E..V....... List the available devices
  -delay             <int>        E..V....... Delay frame output by the given amount of frames (from 0 to INT_MAX) (default INT_MAX)
  -no-scenecut       <boolean>    E..V....... When lookahead is enabled, set this to 1 to disable adaptive I-frame insertion at scene cuts (default false)
  -forced-idr        <boolean>    E..V....... If forcing keyframes, force them as IDR frames. (default false)
  -b_adapt           <boolean>    E..V....... When lookahead is enabled, set this to 0 to disable adaptive B-frame decision (default true)
  -spatial-aq        <boolean>    E..V....... set to 1 to enable Spatial AQ (default false)
  -spatial_aq        <boolean>    E..V....... set to 1 to enable Spatial AQ (default false)
  -temporal-aq       <boolean>    E..V....... set to 1 to enable Temporal AQ (default false)
  -temporal_aq       <boolean>    E..V....... set to 1 to enable Temporal AQ (default false)
  -zerolatency       <boolean>    E..V....... Set 1 to indicate zero latency operation (no reordering delay) (default false)
  -nonref_p          <boolean>    E..V....... Set this to 1 to enable automatic insertion of non-reference P-frames (default false)
  -strict_gop        <boolean>    E..V....... Set 1 to minimize GOP-to-GOP rate fluctuations (default false)
  -aq-strength       <int>        E..V....... When Spatial AQ is enabled, this field is used to specify AQ strength. AQ strength scale is from 1 (low) - 15 (aggressive) (from 1 to 15) (default 8)
  -cq                <float>      E..V....... Set target quality level (0 to 51, 0 means automatic) for constant quality mode in VBR rate control (from 0 to 51) (default 0)
  -aud               <boolean>    E..V....... Use access unit delimiters (default false)
  -bluray-compat     <boolean>    E..V....... Bluray compatibility workarounds (default false)
  -init_qpP          <int>        E..V....... Initial QP value for P frame (from -1 to 51) (default -1)
  -init_qpB          <int>        E..V....... Initial QP value for B frame (from -1 to 51) (default -1)
  -init_qpI          <int>        E..V....... Initial QP value for I frame (from -1 to 51) (default -1)
  -qp                <int>        E..V....... Constant quantization parameter rate control method (from -1 to 51) (default -1)
  -qp_cb_offset      <int>        E..V....... Quantization parameter offset for cb channel (from -12 to 12) (default 0)
  -qp_cr_offset      <int>        E..V....... Quantization parameter offset for cr channel (from -12 to 12) (default 0)
  -weighted_pred     <int>        E..V....... Set 1 to enable weighted prediction (from 0 to 1) (default 0)
  -coder             <int>        E..V....... Coder type (from -1 to 2) (default default)
     default         -1           E..V....... 
     auto            0            E..V....... 
     cabac           1            E..V....... 
     cavlc           2            E..V....... 
     ac              1            E..V....... 
     vlc             2            E..V....... 
  -b_ref_mode        <int>        E..V....... Use B frames as references (from -1 to 2) (default -1)
     disabled        0            E..V....... B frames will not be used for reference
     each            1            E..V....... Each B frame will be used for reference
     middle          2            E..V....... Only (number of B frames)/2 will be used for reference
  -a53cc             <boolean>    E..V....... Use A53 Closed Captions (if available) (default true)
  -dpb_size          <int>        E..V....... Specifies the DPB size used for encoding (0 means automatic) (from 0 to INT_MAX) (default 0)
  -multipass         <int>        E..V....... Set the multipass encoding (from 0 to 2) (default disabled)
     disabled        0            E..V....... Single Pass
     qres            1            E..V....... Two Pass encoding is enabled where first Pass is quarter resolution
     fullres         2            E..V....... Two Pass encoding is enabled where first Pass is full resolution
  -ldkfs             <int>        E..V....... Low delay key frame scale; Specifies the Scene Change frame size increase allowed in case of single frame VBV and CBR (from 0 to 255) (default 0)
  -extra_sei         <boolean>    E..V....... Pass on extra SEI data (e.g. a53 cc) to be included in the bitstream (default true)
  -udu_sei           <boolean>    E..V....... Pass on user data unregistered SEI if available (default false)
  -intra-refresh     <boolean>    E..V....... Use Periodic Intra Refresh instead of IDR frames (default false)
  -single-slice-intra-refresh <boolean>    E..V....... Use single slice intra refresh (default false)
  -constrained-encoding <boolean>    E..V....... Enable constrainedFrame encoding where each slice in the constrained picture is independent of other slices (default false)

