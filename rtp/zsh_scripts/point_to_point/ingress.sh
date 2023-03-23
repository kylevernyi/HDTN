# !/bin/zsh 

config_files=$HDTN_RTP_DIR/config_files/point_to_point
# source_config=$config_files/outducts/ltp_media_source.json
sink_config=$config_files/mediasink_stcp.json
# source_config=$config_files/outducts/mediasource_tcpcl.json

outgoing_rtp_port=40004
raspberrypi_ip=192.168.1.172
cd $HDTN_RTP_DIR 

./build/bprecv_stream  --my-uri-eid=ipn:2.1 --inducts-config-file=$sink_config  --outgoing-rtp-hostname=$raspberrypi_ip \
        --outgoing-rtp-port=$outgoing_rtp_port --num-circular-buffer-vectors=500 --max-outgoing-rtp-packet-size-bytes=1472 \
        --ffmpeg-command="\
        ffmpeg -y -protocol_whitelist data,file,udp,rtp \
        -strict experimental \
        -fflags +genpts \
        -seek2any 1 \
        -avoid_negative_ts +make_zero \
        -max_delay 0 \
        -reorder_queue_size 0 \
        -loglevel verbose \
        -i    raspiberry_PtP.flac"
        # -listen_timeout -1 \
        # ffplay -i  -protocol_whitelist data,file,udp,rtp  -reorder_queue_size 0  -fflags nobuffer+fastseek+flush_packets -sync ext -flags low_delay -framedrop" &
stream_recv_id=$!    