#!/bin/bash
#
# Indent commands to format all source code in a consistent manner
# Please run this script with GNU indent 2.2.10+ before comitting.
#

INDENT=/usr/local/bin/indent

OPTIONS="-nbad -bap -nbc -br -nce -cdw -cli2 -npcs -ncs -nprs -di2 -npsl -l120 \
 -brs -brf -i2 -ci2 -lp -nut -ts2 -bbo -hnl -nprs -nsc -nsob \
 -T MediaScanAudio -T MediaScanImage -T MediaScanVideo -T MediaScanError -T MediaScanResult \
 -T MediaScanProgress -T MediaScanThumbSpec -T MediaScan -T ResultCallback -T ErrorCallback \
 -T MediaScanThread \
 -T ProgressCallback -T FolderChangeCallback -T JPEGData -T PNGData -T buf_src_mgr -T j_common_ptr \
 -T j_decompress_ptr -T j_compress_ptr -T JOCTET -T dlna_t -T av_codecs_t -T pix -T fixed_t -T Buffer -T FILE -T GUID \
 -T uint8_t -T uint16_t -T uint32_t -T uint64_t -T int8_t -T int16_t -T int32_t -T int64_t -T size_t"

$INDENT $OPTIONS src/*.h include/*.h
$INDENT $OPTIONS src/*.c

# We don't reformat external code we've included in subdirectories of src

rm -f src/*.h~ src/*.c~ include/*.h~