config_files=$HDTN_RTP_DIR/config_files/test_3_point_to_point
sink_config=$config_files/mediasink_ltp.json

outgoing_rtp_port=40004 

output_file_path="/home/$USER/test_outputs/test_3"
filename=lucia_crf18                 # change this for whatever file you want to name
file=$output_file_path/$filename

mkdir -p  $output_path/$filename

cd $HDTN_RTP_DIR 

./build/bprecv_stream  --my-uri-eid=ipn:2.1 --inducts-config-file=$sink_config  --outgoing-rtp-hostname=127.0.0.1 \
        --outgoing-rtp-port=$outgoing_rtp_port --num-circular-buffer-vectors=10000 --max-outgoing-rtp-packet-size-bytes=1560 \
        --ffmpeg-command="\
        ffmpeg -y -protocol_whitelist file,udp,rtp,data \
        -strict experimental \
        -fflags +genpts \
        -seek2any 1 \
        -avoid_negative_ts +make_zero \
        -reorder_queue_size 0 \
        -loglevel verbose \
        -fflags nobuffer+fastseek+flush_packets -flags low_delay \
        -vcodec copy -acodec copy \
        -f mp4 $file/$filename.mp4" & 

recv_pid=$!


sleep 400

kill -2 $recv_pid
