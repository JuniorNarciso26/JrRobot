"""Send real C JPEG packets through the real browser reassembler."""
import ast
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile
import unittest
import base64

ROOT = Path(__file__).resolve().parents[2]
HARNESS = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
enum { ESP_PEER_ERR_NONE,ESP_PEER_ERR_WRONG_STATE,ESP_PEER_ERR_INVALID_ARG,ESP_PEER_ERR_WOULD_BLOCK,ESP_PEER_DATA_CHANNEL_DATA };
typedef struct { int type,stream_id;uint8_t *data;int size; } esp_peer_data_frame_t;
static intptr_t peer=1;
static bool session_running=true,video_dc_open=true,force_failure;
static int video_dc_stream_id;
#define pdMS_TO_TICKS(ms) (ms)
static void vTaskDelay(unsigned ticks) { assert(ticks==1); }
static int esp_peer_send_data(intptr_t handle,esp_peer_data_frame_t *frame) {
    assert(handle==1 && frame->size>5 && frame->size<=2053);
    assert(frame->type==ESP_PEER_DATA_CHANNEL_DATA && frame->stream_id==0);
    if(force_failure)return ESP_PEER_ERR_WOULD_BLOCK;
    unsigned n=(unsigned)frame->size;
    uint8_t length[4]={(uint8_t)(n>>24),(uint8_t)(n>>16),(uint8_t)(n>>8),(uint8_t)n};
    assert(fwrite(length,1,4,stdout)==4);
    assert(fwrite(frame->data,1,n,stdout)==n);
    return ESP_PEER_ERR_NONE;
}
'''
MAIN = r'''
int main(void) {
    static uint8_t jpeg[262145],packet[JR_WA_VIDEO_DC_CHUNK_SIZE+5];
    for(size_t i=0;i<sizeof(jpeg);i++)jpeg[i]=(uint8_t)((i*17)%251);
    const size_t sizes[]={1,2047,2048,2049,10000,65536,262144};
    for(unsigned i=0;i<sizeof(sizes)/sizeof(sizes[0]);i++)
        assert(send_jpeg_data_channel(jpeg,sizes[i],packet)==ESP_PEER_ERR_NONE);
    assert(send_jpeg_data_channel(jpeg,262145,packet)==ESP_PEER_ERR_INVALID_ARG);
    force_failure=true;
    assert(send_jpeg_data_channel(jpeg,10000,packet)==ESP_PEER_ERR_WOULD_BLOCK);
    peer=0;
    assert(send_jpeg_data_channel(jpeg,10000,packet)==ESP_PEER_ERR_WRONG_STATE);
    return 0;
}
'''
NODE = r'''
const fs=require('node:fs'),assert=require('node:assert/strict');
const input=JSON.parse(fs.readFileSync(0,'utf8'));
let rtcJpegParts=[],rtcJpegActive=false,rtcJpegNextSeq=0,rtcJpegBytes=0;
let live=true,liveFrames=0,liveStarted=0;
const frames=[],errors=[];
function resetJpegReasm(){rtcJpegParts=[];rtcJpegActive=false;rtcJpegNextSeq=0;rtcJpegBytes=0;}
function setCameraBlob(blob){frames.push(blob);}
function liveLogAdd(line){if(/BAD_LEN|VIDEO_DC_SEQ/.test(line))errors.push(line);}
function liveProfile(){return {label:'test'};}
function q(){return {};}
function cameraRotationLabel(){return '0';}
eval(input.receiver);
(async()=>{
  for(const encoded of input.packets){const data=Uint8Array.from(Buffer.from(encoded,'base64'));await handleVideoDcMessage(data.buffer);}
  assert.equal(errors.length,0);assert.equal(frames.length,input.sizes.length);
  for(let i=0;i<frames.length;i++){
    const expected=Uint8Array.from({length:input.sizes[i]},(_,index)=>(index*17)%251);
    assert.deepEqual(new Uint8Array(await frames[i].arrayBuffer()),expected);
  }
  console.log('C sender -> browser receiver: 7 complete frames, including 128 packets / 262144 bytes');
})().catch(error=>{console.error(error);process.exitCode=1;});
'''


class PacketTests(unittest.TestCase):
    def test_c_sender_and_browser_receiver(self):
        source = (ROOT / "firmware/module_audio/jr_webrtc_audio.c").read_text()
        definitions = "\n".join(re.findall(r"^#define JR_WA_VIDEO_DC_(?:CHUNK_SIZE|HEADER_SIZE|END|SEQ_MASK)\s+[^\n]+", source, re.M))
        start = source.index("static int send_jpeg_data_channel(")
        end = source.index("\nstatic void video_send_task(", start)
        receiver_header = (ROOT / "firmware/module_portal/jr_portal_v16.h").read_text()
        receiver = ast.literal_eval(next(line for line in receiver_header.splitlines()
                                        if line.startswith('"async function handleVideoDcMessage(')))
        with tempfile.TemporaryDirectory() as directory:
            directory = Path(directory)
            (directory / "main.c").write_text(HARNESS + definitions + "\n" + source[start:end] + MAIN)
            executable = directory / "test"
            subprocess.run(["gcc", "-std=c11", "-Wall", "-Wextra", "-Werror",
                            "-fsanitize=address,undefined", str(directory / "main.c"), "-o", str(executable)], check=True)
            result = subprocess.run([str(executable)], capture_output=True,
                                    env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
            self.assertEqual(result.returncode, 0, result.stderr.decode())
        packets = []
        position = 0
        while position < len(result.stdout):
            length = int.from_bytes(result.stdout[position:position + 4], "big")
            self.assertLessEqual(length, 2053)
            position += 4
            packet = result.stdout[position:position + length]
            self.assertEqual(len(packet), length)
            packets.append(base64.b64encode(packet).decode())
            position += length
        subprocess.run(["node", "-e", NODE], input=json.dumps({
            "receiver": receiver, "packets": packets,
            "sizes": [1, 2047, 2048, 2049, 10000, 65536, 262144],
        }), text=True, check=True)


if __name__ == "__main__":
    unittest.main()
