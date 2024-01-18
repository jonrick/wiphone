/*
Copyright © 2019, 2020, 2021, 2022, 2023 HackEDA, Inc.
Licensed under the WiPhone Public License v.1.0 (the "License"); you
may not use this file except in compliance with the License. You may
obtain a copy of the License at
https://wiphone.io/WiPhone_Public_License_v1.0.txt.

Unless required by applicable law or agreed to in writing, software,
hardware or documentation distributed under the License is distributed
on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
either express or implied. See the License for the specific language
governing permissions and limitations under the License.
*/

#ifndef  LORA_H
#define LORA_H

#include "Hardware.h"
#include <SPI.h>
#include <RH_RF95.h>
#include <RHSoftwareSPI.h>
#include <RadioHead.h>
#include "tinySIP.h"
#include "Storage.h"

#define LORA_DST_UNREACH  -5
#define LORA_MAX_UNACK_SEQ 100
/*Define LORA FLAG Types*/
#define LORA_ACK  2
#define LORA_OLD_ACK 6
#define LORA_SND  1
#define LORA_SNDW_PAD 0x0011
#define LORA_FIN  256
#define LORA_NCK  0
#define LORA_TBC  5
#define LORA_ACK_NC 3
#define LORA_SND_ACK 4

#define LORA_MESSAGE_MIN_LEN (sizeof(uint32_t) * 2) + (sizeof(uint8_t) * 2)
#define LORA_MESSAGE_MAGIC 0x6c6d
#define LORA_MAX_MESSAGE_LEN 220
#define LORA_ARQ_PKT_LEN     243

class Lora {
public:
  Lora();
  virtual ~Lora();
  void setup();


  TextMessage* parse_message(const uint8_t *message, uint8_t len, uint8_t *flagRX, uint16_t *IDRX, uint16_t *seqRX, uint32_t *payloadSizeRX);
  TextMessage* parse_old_message(const uint8_t *message, uint8_t len);
  int send_old_message(const char* to, const char* message);
  int loop();
  int send_message(const char* to, const char* message);
  int send_ack(const char* to, uint16_t idACK, uint16_t seqACK, uint32_t paylSizeACK);
  void detachInterrupts();
  void attachInterrupts();
  //RHGenericDriver::RHMode getMode();
  void setFrequency(float freq);
  void setOnOff(bool isOn);
  bool chkRxedSeq(uint16_t seq);
  void reset();
  uint8_t old_lora_ack_flag = 0;
  uint16_t msgBlks = 0;
  bool isOn() {
    return mIsOn;
  }
  void setSendUndelivereds(bool val) {
    mSendUndelivereds = val;
  }
  bool isSendUndelivereds() {
    return mSendUndelivereds;
  }

  uint16_t * seqNCK() {
    return unACKseq;
  }

  uint16_t getParsSeq() {
    return currSeq;
  }

  void setParseSeq(uint16_t seqTmp) {
    currSeq = seqTmp;
  }

  uint32_t getParseID() {
    return currID;
  }

  void setParseID(uint32_t IDTmp) {
    currID = IDTmp;
  }

  uint8_t getParseFlag() {
    return currFlag;
  }

  void setParseFlag(uint8_t flagTmp) {
    currFlag = flagTmp;
  }

  uint32_t getParsePaylSize() {
    return currpaylSize;
  }

  void setParsePaylSize(uint32_t paylSizeTmp) {
    currpaylSize = paylSizeTmp;
  }

  uint16_t getRxedSeq() {
    return seqCnt;
  }

  void serRxedSeq(uint16_t tmpSeq) {
    seqCnt = tmpSeq;
  }
  uint16_t getMsgBlks() {
    if (currpaylSize < LORA_MAX_MESSAGE_LEN) {
      return 1;
    }
    uint16_t msgBlocks = currpaylSize/LORA_MAX_MESSAGE_LEN;
    if ((currpaylSize%LORA_MAX_MESSAGE_LEN) != 0) {
      msgBlocks++;
    }
    return msgBlocks;
  }

  void setTxSeq(uint16_t seqtx) {
    seqTx = seqtx;
  }

  uint16_t getTxSeq() {
    return seqTx;
  }

  uint16_t getTxCnt() {
    return TxCnt;
  }

  void setTxCnt(uint8_t cntTx) {
    TxCnt = cntTx;
  }

  uint8_t getTxRes() {
    return resTx;
  }

  void setTxRes(uint8_t txres) {
    resTx = txres;
  }

  void setOldTxCnt(uint8_t oldCnt) {
    oldTxCnt = oldCnt;
  }

  uint8_t getOldTxCnt() {
    return oldTxCnt;
  }
private:
  RHSoftwareSPI* loraSPI;
  RH_RF95* rf95;
  bool mIsOn;
  bool mSendUndelivereds;
  uint16_t unACKseq[LORA_MAX_UNACK_SEQ] = {0};
  uint16_t currSeq;
  uint16_t currID;
  uint16_t currFlag;
  uint32_t currpaylSize;
  uint16_t seqCnt = 0;
  uint16_t seqTx = 0;
  uint8_t TxCnt = 0;
  uint8_t resTx;
  uint8_t oldTxCnt = 0;
  uint8_t old_lora_cnt = 0;
  uint8_t chkACK = 0;

  //handshaking constants
  const char* LORA_ACK_MESSAGE_PREFIX = "[[[LORA_ACK,";
  const int LORA_ACK_MESSAGE_PREFIX_LEN = 12;
  //uint32_t timeOfLastMessageSent;
  char acknowledgeMessage[300];//TODO the max msg length should be considered
  char acknowledgeSender[100];
  char acknowledgeText[300];//TODO the maximum message length shuld be considered here
  char* msgAccum;
  //bool parseAcknowledgeMsg(char* msg, char* sender, char* text);

  uint16_t parseAcknowledgeMsg(char *msgrx, uint16_t id, uint16_t seq);
};

#endif
