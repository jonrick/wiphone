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

#include "lora.h"
#include "GUI.h"
#include "ErrorsPopupStrings.h"

typedef struct __attribute__((packed)) {
  uint16_t magic;  // WiPhone magic number (2 bytes)
  uint32_t to;  // the dst of the message (4 bytes)
  uint32_t from;  // the src of message (4 bytes)
  char     message[LORA_MAX_MESSAGE_LEN];  // payload to be sent (220 bytes)
  uint16_t id;  // container for same message (2 bytes)
  uint16_t seq;  // number of blocks per message (each is LORA_MAX_MESSAGE_LEN) (2 bytes)
  uint8_t flag;  // flag is 1 for send and 2 for ack a seq (1 byte)
  uint32_t payloadSize;  // the total size of payload to allow receiver calculate the number of expected blocks (4 bytes)
  uint32_t reserved = 0xFFFFFFFF;  // future use (4 bytes)
}
lora_message;

extern uint32_t chipId;
extern GUI gui;
uint32_t *seqChkArr;
Lora::Lora() {

}
Lora::~Lora() {
  delete loraSPI;
  delete rf95;
}
void Lora::detachInterrupts() {
  rf95->detachInterrupts();
}

void Lora::attachInterrupts() {
  rf95->attachInterrupts();
}
/*
RHGenericDriver::RHMode Lora::getMode() {
    return rf95->getMode();
}*/

void Lora::setup() {
#ifdef LORA_MESSAGING
  log_i("Initialising LoRa: %d", ESP.getFreeHeap());

  loraSPI = new RHSoftwareSPI();
  rf95 = new RH_RF95(RFM95_CS, RFM95_INT, *loraSPI);

  loraSPI->setPins(HSPI_MISO, HSPI_MOSI, HSPI_SCLK);
  pinMode(RFM95_RST, OUTPUT);
  /*
    Important note, the WiPhone will be frozed if the pinMode call for ENABLE_DAUGHTER_3V3 is absent,
    and/or trying to send a msg by calling rf95->send after calling allDigitalWrite(ENABLE_DAUGHTER_3V3, LOW).
  */
  // pinMode(ENABLE_DAUGHTER_3V3, OUTPUT);
  // allDigitalWrite(ENABLE_DAUGHTER_3V3, HIGH);
  setOnOff(true);
  if (!rf95->init()) {
    log_d("LORA INIT FAILED <<<<<<<<<<<>>>>>>>>>");
  }
  if (!rf95->setFrequency(RF95_FREQ)) {
    log_d("LORA SET FREQ FAILED <<<<<<<<<>>>>>>>>>>");
  }
  rf95->setTxPower(23, false);

  log_v("Free memory after LoRa: %d %d", ESP.getFreeHeap(), heap_caps_get_free_size(MALLOC_CAP_32BIT));
  setOnOff(false);
  //add un-delivered messages into the outgoing LoRa messages list.
  // gui.flash.messages.addUndeliveredMessagesToTheList(gui.state.outgoingLoraMessages);
#endif
}

void Lora::reset() {

  rf95->setModeTx();
  rf95->setModeRx();

}

void Lora::setFrequency(float freq) {
  rf95->init();
  if (rf95->setFrequency(freq)) {
    log_d("LoRa Frequency set to %f .", freq);
  }
}

void Lora::setOnOff(bool isOn) {
  mIsOn = isOn;
  if(!isOn) {
    log_d("LoRa will be turned OFF >>>>>>>>>>>");
    //rf95->sleep();
    /*
      Important note, the WiPhone will be frozed if this pinMode call is absent,
      and/or trying to send a msg by calling rf95->send after calling allDigitalWrite(ENABLE_DAUGHTER_3V3, LOW).
    */
    allDigitalWrite(ENABLE_DAUGHTER_3V3, LOW);
  } else {

    allDigitalWrite(ENABLE_DAUGHTER_3V3, LOW);
    delay(100);
    allDigitalWrite(ENABLE_DAUGHTER_3V3, HIGH);
    delay(5);

  }
}

TextMessage* Lora::parse_old_message(const uint8_t *message, uint8_t len) {
  /**
   * Message format:
   * uint16_t - magic number (0x6c6d)
   * uint32_t - to address
   * uint32_t - from address
   * char*    - message text (null terminated string)
   */
  lora_message *msg = (lora_message*)message;

  if (len >= LORA_MESSAGE_MIN_LEN && msg->magic == LORA_MESSAGE_MAGIC) {
    if (msg->to != chipId) {
      if (msg->to > 0 ) {
        return NULL; // Message not for us
      }
    }

    char to_str[30] = {0}, from_str[30] = {0};
    snprintf(to_str, sizeof(to_str), "LORA:%X", msg->to);
    snprintf(from_str, sizeof(from_str), "LORA:%X", msg->from);

    log_i("LoRa message: to: %s from: %s msg: %s", to_str, from_str, msg->message);

    return new TextMessage(msg->message, from_str, to_str, 1604837104);
  }

  return NULL;
}


TextMessage* Lora::parse_message(const uint8_t *message, uint8_t len, uint8_t *flagRX, uint16_t *IDRX, uint16_t *seqRX, uint32_t *payloadSizeRX) {
  /**
   * Message format:
   * uint16_t - magic number (0x6c6d)
   * uint32_t - to address
   * uint32_t - from address
   * char*    - message text (null terminated string)
   * uint16_t - container for same message
   * uint16_t - number of blocks per message (each is LORA_MAX_MESSAGE_LEN)
   * uint8_t  - flag is 1 for send and 2 for ack a seq
   * uint32_t - the total size of payload to allow receiver calculate the number of expected blocks
   * uint32_t - reserved for future use
   */
  lora_message *msg = (lora_message*)message;

  if (len >= LORA_MESSAGE_MIN_LEN && msg->magic == LORA_MESSAGE_MAGIC) {
    if (msg->to != chipId) {
      if (msg->to > 0 ) {
        return NULL; // Message not for us
      }
    }

    *flagRX = msg->flag;
    *IDRX = msg->id;
    *seqRX = msg->seq;
    *payloadSizeRX = msg->payloadSize;

    char to_str[30] = {0}, from_str[30] = {0};
    snprintf(to_str, sizeof(to_str), "LORA:%X", msg->to);
    snprintf(from_str, sizeof(from_str), "LORA:%X", msg->from);

    log_i("LoRa message: to: %s from: %s msg: %s", to_str, from_str, msg->message);

    return new TextMessage(msg->message, from_str, to_str, 1604837104);
  }

  return NULL;
}
// TextMessage* Lora::parse_message(const uint8_t *message, uint8_t len, uint16_t tmpSeq) {
//   /**
//    * Message format:
//    * uint16_t - magic number (0x6c6d)
//    * uint32_t - to address
//    * uint32_t - from address
//    * char*    - message text (null terminated string)
//    */
//   lora_message *msg = (lora_message*)message;
//   tmpSeq = 0;
//   if (len >= LORA_MESSAGE_MIN_LEN && msg->magic == LORA_MESSAGE_MAGIC) {
//     if (msg->to != chipId) {
//       if (msg->to > 0 ) {
//         return NULL; // Message not for us
//       }
//     }
//     uint16_t id = msg->id;
//     uint16_t seq = msg->seq;
//     uint8_t flag = msg->flag;
//     uint32_t payloadSize = msg->payloadSize;
//     uint16_t nuBlocks = payloadSize/LORA_MAX_MESSAGE_LEN;  // Calculate the number of blocks of the message
//     if (payloadSize%LORA_MAX_MESSAGE_LEN != 0) {  // check if there is not complete block
//       nuBlocks++;
//     }
//     if (flag == 2) {
//       if ((id+1+nuBlocks) == seq) {
//         char to_str[30] = {0}, from_str[30] = {0};
//         snprintf(to_str, sizeof(to_str), "LORA:%X", msg->to);
//         snprintf(from_str, sizeof(from_str), "LORA:%X", msg->from);
//         log_i("LoRa message: to: %s from: %s msg: %s", to_str, from_str, msg->message);
//         return new TextMessage(msg->message, from_str, to_str, 1604837104);
//       } else {
//         // return
//       }
//     }
//   }
//   return NULL;
// }

int Lora::loop() {
  // uint32_t seqCnt=0;  // counter for array packing with rxed seq to be checked when ever rx to prevent msg duplication

  if (mIsOn && rf95->available()) {
    uint8_t tmpFlag = 0;
    uint16_t tmpID = 0;
    uint16_t tmpSeq = 0;
    uint32_t tmpPayloadSize = 0;
    uint16_t msgBlocks = 0;
    uint16_t tmpRxedID = 0;
    // Should be a message for us now
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN] = {0};
    uint8_t len = sizeof(buf);

    if (rf95->recv(buf, &len)) {
      // log_d("Lora received a message len %d>>>>>>>>>>>",len);
      if (len != LORA_ARQ_PKT_LEN) {

        TextMessage *msg = parse_old_message(buf, len);
        if(strncmp(msg->message, LORA_ACK_MESSAGE_PREFIX, LORA_ACK_MESSAGE_PREFIX_LEN) == 0) {
          chkACK = LORA_OLD_ACK;
          return LORA_OLD_ACK;
        }


        sprintf(acknowledgeMessage, "%sText:%s,Sender:%s]]]\0", LORA_ACK_MESSAGE_PREFIX, msg->message, msg->to);
        old_lora_cnt = (old_lora_cnt + 1)%3;
        log_d("old lora ack is %d >>>>>>", old_lora_cnt);

        // for(int i=0; i<10; i++) {
        //   send_old_message(msg->from+5, acknowledgeMessage);
        // }



        // if (old_lora_cnt == 1) {
        //log_d("old lora ack is %d >>>>>>", old_lora_cnt);
        gui.flash.messages.saveMessage(msg->message, msg->from, msg->to, true, ntpClock.getExactUtcTime());
        // } else if (old_lora_cnt == 2) {
        //   old_lora_cnt = 0;
        // }

        return LORA_SND;
      }

      TextMessage *msg = parse_message(buf, len, &tmpFlag, &tmpID, &tmpSeq, &tmpPayloadSize);

      if (msg != NULL) {

        /*extarct the message metadata*/
        setParseSeq(tmpSeq);
        setParseID(tmpID);
        setParseFlag(tmpFlag);
        setParsePaylSize(tmpPayloadSize);

        if(tmpFlag == LORA_ACK) {
          log_d("LORA Received an ACK message");
          chkACK = LORA_ACK;
          return LORA_ACK;
        }
        msgBlocks = tmpPayloadSize/LORA_MAX_MESSAGE_LEN;
        if ((tmpPayloadSize%LORA_MAX_MESSAGE_LEN) != 0) {
          msgBlocks++;
        }

        bool testSeq = chkRxedSeq(tmpSeq);  // check if the seq already existed or not
        unACKseq[seqCnt%100] = tmpSeq;
        seqCnt = (seqCnt + 1)%0xFFFFFFFF;

        if (msgBlocks == 1) {

          if (tmpFlag == LORA_SND) {
            bool detect = rf95->waitCAD();
            // log_d("channel is avialable now and detcet is %d>>>>>", detect);
            if (!testSeq) {
              gui.flash.messages.saveMessage(msg->message, msg->from, msg->to, true, ntpClock.getExactUtcTime()); // time == 0 for unknown real time
            }

            if (send_ack(msg->to, tmpID, tmpSeq, tmpPayloadSize )) {
              log_d("ACK for MSG seq no %d is sent", tmpSeq);
            }
            // log_d("Message received has one block and it will be saved>>>>>>>>");

            return LORA_SND;
          }
        }

        if(!testSeq) {
          //log_d("ack or normal msg: %s", msg->message);
          //gui.showPopup("LORA Msg Recvd", "Message", "", "", "Back", "OK", (unsigned char*)icon_phone_small_w_crossed, sizeof(icon_phone_small_w_crossed));
          if (tmpFlag == LORA_SND) {  // the flag is send
            if(((tmpID+msgBlocks) == tmpSeq) && (tmpRxedID == tmpID)) {  // the last message seq for same id
              // log_d("send last ACK>>>>>>");
              strlcat(msgAccum, msg->message, sizeof(msg->message));
              gui.flash.messages.saveMessage(msgAccum, msg->from, msg->to, true, ntpClock.getExactUtcTime()); // time == 0 for unknown real time
              send_ack(msg->to, tmpID, tmpSeq, tmpPayloadSize );
              delete msgAccum;
              delete msg;
              setParseSeq(0);
              setParseID(0);
              setParseFlag(0);
              setParsePaylSize(0);
              seqCnt = 0;
              return LORA_SND;

            } else if (tmpSeq == (tmpID+1)) {  // the first message and then we assign the tmpRxedID to track its all seq messages
              log_d("send first ACK");
              tmpRxedID = tmpID;
              strlcpy(msgAccum, msg->message, sizeof(msg->message));
              send_ack(msg->to, tmpID, tmpSeq, tmpPayloadSize );
              delete msg;
              return LORA_TBC;
            } else if ( (tmpID < tmpSeq) && (tmpRxedID == tmpID)) {  // check the tmpseq for same id
              log_d("send ACK TBC");
              strlcat(msgAccum, msg->message, sizeof(msg->message));
              send_ack(msg->to, tmpID, tmpSeq, tmpPayloadSize );
              delete msg;
              return LORA_TBC;
            }
          }
        }

      }
    } else {
      log_e("LoRa: Unable to receive data");
    }
  }

  return LORA_NCK;
}

int Lora::send_old_message(const char* to, const char* message) {
  if(!mIsOn) {
    /*
      Important note, the WiPhone will be frozen if trying to send a msg by calling rf95->send
      after calling allDigitalWrite(ENABLE_DAUGHTER_3V3, LOW).
    */
    log_d("LoRa error: sending message while LoRa is OFF!");
    return -1;
  }

  log_d("send_message: %s to: %s", message, to);
  if (strlen(message) > LORA_MAX_MESSAGE_LEN) {
    log_e("Unable to send LoRa message - too large: %d", strlen(message));
    return -1;
  }

  lora_message msg;
  msg.magic = LORA_MESSAGE_MAGIC;
  msg.from = chipId;
  msg.to = strtol(to, NULL, 16);
  strlcpy(msg.message, message, LORA_MAX_MESSAGE_LEN);

  if(!rf95->send((uint8_t*)&msg, strlen(message)+LORA_MESSAGE_MIN_LEN+1)) {
    log_e("LoRa message COULD NOT be sent to: %X from: %X", msg.to, msg.from);
    return 0;
  }
  // rf95->waitPacketSent();

  log_d("LoRa message sent to: %X from: %X", msg.to, msg.from);
  return true;
}


int Lora::send_message(const char* to, const char* message) {
  CriticalFile ini(Storage::ConfigsFile);
  if(!mIsOn) {
    /*
      Important note, the WiPhone will be frozen if trying to send a msg by calling rf95->send
      after calling allDigitalWrite(ENABLE_DAUGHTER_3V3, LOW).
    */
    log_d("LoRa error: sending message while LoRa is OFF!");
    return -1;
  }
  uint16_t numberBlocks = 0;
  // log_d("send_message: %s to: %s", message, to);
  if (strlen(message) > LORA_MAX_MESSAGE_LEN) {
    // need to calculate the number of blocks
    numberBlocks = strlen(message)/LORA_MAX_MESSAGE_LEN;

    if ((strlen(message)%LORA_MAX_MESSAGE_LEN) != 0) {  // add more block for the remainder
      numberBlocks++;
    }
  } else if ((strlen(message) < LORA_MAX_MESSAGE_LEN) || (strlen(message) == LORA_MAX_MESSAGE_LEN)) {
    numberBlocks = 1;
  }

  lora_message msg;
  msg.magic = LORA_MESSAGE_MAGIC;  // pack wiphone magic number
  msg.from = chipId;  // pack src addr
  msg.to = strtol(to, NULL, 16);  // pack dst addr
  msg.payloadSize = strlen(message);  // pack payload size
  msg.flag = LORA_SND;
  if (TxCnt == 0) {
    // log_d("new msg seq generation >>>>>>>>>>");
    msg.seq =  Random.random() & 0xFFFF;
    setTxSeq(msg.seq);
    msg.id = msg.seq - 1;
  } else {
    // log_d("same message seq usage >>>>>>");
    msg.seq =  getTxSeq();

    msg.id = msg.seq - 1;
  }

  int res = 0;
  for (int blkCnt = 0; blkCnt < numberBlocks; blkCnt++) {  // rotate to send the blocks of the message
    // log_d("Message Block number %d is being sent>>>>>>>>>>", blkCnt);
    //for (uint8_t loraSndCnt = 0; loraSndCnt < 5; loraSndCnt++) {  // loop to increase wait time after each request

    //log_d("Message TXion Trial is %d >>>>>>>>>>",loraSndCnt);

    // if (getTxCnt() != (getOldTxCnt() + 1)) {
    //   setOldTxCnt(getTxCnt());
    strlcpy(msg.message, ((&message[0])+(blkCnt*LORA_MAX_MESSAGE_LEN)), LORA_MAX_MESSAGE_LEN);
    if(!rf95->send((uint8_t*)&msg, LORA_ARQ_PKT_LEN)) {
      setOnOff(false);
      log_e("LoRa message COULD NOT be sent to: %X from: %X", msg.to, msg.from);

      ini.unload();
      if ((ini.load() || ini.restore()) && !ini.isEmpty()) {
        log_d("WiPhone reinit the lora");

        if (ini[0].hasKey("v") && strcmp(ini[0]["v"], "1") == 0) {
          log_d("check init of lora");
          if (!ini.hasSection("lora")) {
            log_d("lora ini file not exist");
            setOnOff(true);
            setFrequency((float)915.0 );// can be defined in a header

          } else {
            log_d("Lora Section exist");
            setOnOff((strcmp(ini["lora"]["onOff"], "On") == 0) ? true : false);
            setFrequency((strcmp(ini["lora"]["freq"], "915Mhz") == 0) ? (float)915.0 : (float)868.0);// can be defined in a header
            // lora.setSendUndelivereds((strcmp(ini["lora"]["resendUndelivereds"], "No") == 0) ? false : true);

          }


        }
      }
      ini.unload();
      return 0;
    }
    // log_d("Lora wait for ACK >>>>>>>>>");
    // rf95->waitPacketSent();  // wait lora complete send
    // }
    if (!chkACK) {
      for (int loopCnt = 0; loopCnt < 100; loopCnt++) {
        res = loop();

        if (res == LORA_ACK) {
          log_d("RX ACK res is %d >>>>>", res);
          loopCnt = 100;

          break;
        }
        if (res == LORA_OLD_ACK) {
          // log_d("RX OLD ACK res is %d >>>>>", res);
          old_lora_ack_flag = 0xa;

          return true;
        }
      }

    } else {
      log_d("RX ACK res is %d >>>>>", chkACK);

      if (chkACK == LORA_OLD_ACK) {

        old_lora_ack_flag = 0xa;
        chkACK = 0;
        return true;
      }
    }

    uint16_t seqParsd = getParsSeq();
    log_d("Message sent Seq %d and ack seq is %d>>>>>>>.",msg.seq, seqParsd);
    if (((res == LORA_ACK) || ((chkACK == LORA_ACK))) && (seqParsd == msg.seq)) {
      chkACK = 0;
      log_d("Message with Seq %d has received an ACK>>>>>>>.",msg.seq);
      seqCnt++;

      break;
    } else if ( (getTxCnt() == 5)) {  // wiphone should display an error that target is not reachable
      log_d("Message with Seq %d has not received an ACK>>>>>>>.",msg.seq);
      setTxCnt(6);  // set Send trial to 5 to terminate the session of catching the ack to allow popup of warn message
      // gui.showPopup(MSG_WARN_POPUP_HEADER,
      //                     MSG_ERR_POPUP_10_1ST_LINE,
      //                     MSG_ERR_POPUP_10_2ND_LINE,
      //                     MSG_ERR_POPUP_10_3RD_LINE, "", "OK", (unsigned char*)icon_message_w, sizeof(icon_message_w));
      break;  // it should continue to send the rest of message as old FW hasn't the same ARQ mechanism
    }
  }
  return true;
}

int Lora::send_ack(const char* to, uint16_t idACK, uint16_t seqACK, uint32_t paylSizeACK) {
  lora_message msg;

  msg.magic = msg.magic = LORA_MESSAGE_MAGIC;  // pack wiphone magic number
  msg.from = chipId;  // pack src addr
  msg.to = strtol(to, NULL, 16);  // pack dst addr
  msg.payloadSize = paylSizeACK;  // pack payload size
  msg.id = idACK;
  msg.seq = seqACK;
  msg.flag = LORA_ACK;
  char messageACK[230] = {0xFF};

  if(!rf95->send((uint8_t*)&msg, LORA_ARQ_PKT_LEN)) {
    log_e("LoRa message COULD NOT be sent to: %X from: %X", msg.to, msg.from);
    return 0;
  }

  return true;
}

// bool Lora::parseAcknowledgeMsg(char* msg, char* sender, char* text) {
//   if(!msg || !sender || !text) {
//     return false;
//   }
//   char *aField, *msgToIterate, *toFree;
//   toFree = msgToIterate = strdup(msg);
//   //Format: [[[LORA_ACK,Text:abcd efgh,Sender:someOne]]]
//   while ((aField = strsep(&msgToIterate, ","))) {
//     if(!strncmp(aField, "Text:", 5)) {
//       aField += 5;
//       int len = strlen(aField);
//       strncpy(text, aField, len);
//       text[len] = 0;//null terminated string
//     } else if(!strncmp(aField, "Sender:", 7)) {
//       aField += 7;
//       int len = strlen(aField) - 3;// omit ]]]
//       strncpy(sender, aField, len);
//       sender[len] = 0;//null terminated string
//     }
//   }
//   free(toFree);
// }

bool Lora::chkRxedSeq(uint16_t seq) {
  for (int chkCnt = 0; chkCnt < LORA_MAX_UNACK_SEQ; chkCnt++) {
    if (unACKseq[chkCnt] == seq) {
      return true;
    }
  }
  return false;
}

uint16_t Lora::parseAcknowledgeMsg(char *msgrx, uint16_t id, uint16_t seq) {
  if(id == 0 || seq == 0) {
    return false;
  }

  lora_message *msg = (lora_message*)msgrx;

  uint16_t idRx = msg->id;
  uint16_t seqRx = msg->seq;
  uint8_t flagRx = msg->flag;
  uint32_t payloadSizeRx = msg->payloadSize;

  uint16_t nuBlocks = payloadSizeRx/LORA_MAX_MESSAGE_LEN;  // Calculate the number of blocks of the message
  if (payloadSizeRx%LORA_MAX_MESSAGE_LEN != 0) {  // check if there is not complete block
    nuBlocks++;
  }

  if (flagRx == LORA_ACK && (seq != 0) && (id != 0)) {
    if (idRx == id) {
      if (((idRx+1+nuBlocks) == seqRx) && seqRx == seq) {
        return (seq+LORA_FIN);  // return the seq + LORA_FIN to inform that this is the final packet
      } else {
        return seq;  // return the seq of the sent packet
      }
    } else if (idRx != id) {
      return LORA_NCK;
    }

  }

  return LORA_NCK;
}
