# !/bin/zsh 

config_files=$HDTN_RTP_DIR/config_files/udp/test_2_hdtn_loopback
source_config=$config_files/mediasource_udp.json

test_files=/home/$USER/test_media/official_test_media
filename=lucia_cbr21
file=$test_files/$filename.mp4

incoming_rtp_port=29999

shm_socket_path=/tmp/hdtn_gst_shm_induct_socket

cd $HDTN_RTP_DIR

rm $shm_socket_path

./build/bpsend_stream  --bundle-size=65535 --bundle-rate=0 --use-bp-version-7 \
        --my-uri-eid=ipn:1.1 --dest-uri-eid=ipn:2.1 --outducts-config-file=$source_config \
        --max-incoming-udp-packet-size-bytes=1460 --incoming-rtp-stream-port=$incoming_rtp_port --num-circular-buffer-vectors=2000 \
        --rtp-packets-per-bundle=1 --induct-type="appsink" --file-to-stream=$file  &
bpsend_stream_process=$!


sleep 65

echo "\nkilling video process ..." && kill -2 $bpsend_stream_process

