/*
 * This software implements an IEC 60870-5-104 protocol tester.
 * Copyright ?2010,2011,2012 Ricardo L. Olsen
 *
 * Disclaimer
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#pragma once

// IEC 60870-5-104 BASE CLASS, MASTER IMPLEMENTATION

#include "iec104_types.h"

struct iec_obj
{
    uint32_t address; // 3 byte address

    float value; // value

    cp56time2a timetag; // 7 byte time tag
    uint8_t reserved;   // for future use

    uint8_t type;  // iec type
    uint8_t cause; //
    uint16_t ca;   // common addres of asdu

    union
    {
        uint8_t ov : 1;  // overflow/no overflow
        uint8_t sp : 1;  // single point information
        uint8_t dp : 2;  // double point information
        uint8_t scs : 1; // single command state
        uint8_t dcs : 2; // double command state
        uint8_t rcs : 2; // regulating step command
    };                   // 2 bits

    uint8_t qu : 5; // qualifier of command
    uint8_t se : 1; // select=1 / execute=0
    // + 6 bits = 8 bits

    uint8_t bl : 1; // blocked/not blocked
    uint8_t sb : 1; // substituted/not substituted
    uint8_t nt : 1; // not topical/topical
    uint8_t iv : 1; // valid/invalid
    uint8_t t : 1;  // transient flag
    uint8_t pn : 1; // 0=positive, 1=negative
};

class iec104_class
{
public:
    const uint32_t M_SP_NA_1 = 1;   // single-point information
    const uint32_t M_DP_NA_1 = 3;   // double-point information
    const uint32_t M_ST_NA_1 = 5;   // step position information
    const uint32_t M_BO_NA_1 = 7;   // bitstring of 32 bits
    const uint32_t M_ME_NA_1 = 9;   // normalized value
    const uint32_t M_ME_NB_1 = 11;  // scaled value
    const uint32_t M_ME_NC_1 = 13;  // floating point
    const uint32_t M_IT_NA_1 = 15;  // integrated totals
    const uint32_t M_ME_ND_1 = 21;  //
    const uint32_t M_SP_TB_1 = 30;  // single-point information with time tag
    const uint32_t M_DP_TB_1 = 31;  // double-point information with time tag
    const uint32_t M_ST_TB_1 = 32;  // step position information with time tag
    const uint32_t M_BO_TB_1 = 33;  // bitstring of 32 bits with time tag
    const uint32_t M_ME_TD_1 = 34;  // normalized value with time tag
    const uint32_t M_ME_TE_1 = 35;  // scaled value with time tag
    const uint32_t M_ME_TF_1 = 36;  // floating point with time tag
    const uint32_t M_IT_TB_1 = 37;  // integrated totals with time tag
    const uint32_t C_SC_NA_1 = 45;  // single command
    const uint32_t C_DC_NA_1 = 46;  // double command
    const uint32_t C_RC_NA_1 = 47;  // regulating step command
    const uint32_t C_SC_TA_1 = 58;  // single command with time tag
    const uint32_t C_DC_TA_1 = 59;  // double command with time tag
    const uint32_t C_RC_TA_1 = 60;  // regulating step command with time tag
    const uint32_t M_EI_NA_1 = 70;  // end of initialization
    const uint32_t C_IC_NA_1 = 100; // general interrogation (GI)
    const uint32_t C_CI_NA_1 = 101; // counter interrogation
    const uint32_t C_CS_NA_1 = 103; // clock synchronization command
    const uint32_t C_RP_NA_1 = 105; // reset process command
    const uint32_t C_TS_TA_1 = 107; // test command with time tag CP56Time2a

    /* cause of transmition (standard) */
    const uint32_t CYCLIC = 1;
    const uint32_t BGSCAN = 2;
    const uint32_t SPONTANEOUS = 3;
    const uint32_t REQUEST = 5;
    const uint32_t ACTIVATION = 6;
    const uint32_t ACTCONFIRM = 7;
    const uint32_t DEACTIVATION = 8;
    const uint32_t ACTTERM = 10;
    const uint32_t SUPERVISORY = 0x01;
    const uint32_t STARTDTACT = 0x07;
    const uint32_t STARTDTCON = 0x0B;
    const uint32_t STOPDTACT = 0x13;
    const uint32_t STOPDTCON = 0x23;
    const uint32_t TESTFRACT = 0x43;
    const uint32_t TESTFRCON = 0x83;
    const uint32_t INTERROGATION = 0x64;
    const uint32_t INTEGRATEDTOTALS = 0x65;
    const uint32_t START = 0x68;
    const uint32_t RESET = 0x69;
    const uint32_t POSITIVE = 0;
    const uint32_t NEGATIVE = 1;
    const uint32_t SELECT = 1;
    const uint32_t EXECUTE = 0;

    // ---- user called funcions, must be called by the user -----------------
    iec104_class();         // user called constructor on derived class
    void onConnectTCP();    // user called, when tcp connected
    void onDisconnectTCP(); // user called, when tcp disconnected
    void onTimerSecond();   // user called, each second timer
    void packetReadyTCP();  // user called, when packet ready to be read from tcp connection

    void solicitGI(); // General Interrogation
    void solicitIntegratedTotal();
    void ReadIntegratedTotal();
    void setSecondaryIP(char *ip);
    char *getSecondaryIP();
    void setSecondaryAddress(int addr);
    int getSecondaryAddress();
    void setPrimaryAddress(int addr);
    int getPrimaryAddress();
    void disableSequenceOrderCheck(); // allow sequence out of order
    bool sendCommand(iec_obj *obj);   // Command, return false if not send
    int getPortTCP();
    void setPortTCP(unsigned port);
    void setGICountDown(const int timeout);

private:
    uint16_t VS;            // sender packet control counter
    uint16_t VR;            // receiver packet control counter
    void confTestCommand(); // test command activation confirmation
    void sendStartDTACT();  // send STARTDTACT
    int tout_startdtact;    // timeout control
    void sendSupervisory(); // send supervisory window control frame
    int tout_supervisory;   // countdown to send supervisory window control
    int tout_gi;            // countdown to send general interrogation
    int tout_testfr;        // countdown to send test frame
    bool connectedTCP;      // tcp connection state
    bool seq_order_check;   // if set: test message order, disconnect if out of order
    uint8_t masterAddress;  // master link address (primary address, originator address, oa)
    uint16_t slaveAddress;  // slave link address (secondary address, common address of ASDU, ca)
    unsigned Port;          // iec104 tcp port (defaults to 2404)
    char slaveIP[20];       // slave (secondary, RTU) IP address

    // modified to fix bugs when using this class in mutithread
    const int t3_testfr;
    const int t2_supervisory;
    const int t1_startdtact;

    // Big/little endian reverse
    void reverse(char *first, int size);

protected:
    void parseAPDU(iec_apdu *papdu, int sz, bool accountandrespond = true); // parse APDU, ( accountandrespond == false : process the apdu out of the normal handshake )

    int msg_supervisory;

    bool TxOk;            // ready to transmit state (STARTDTCON received)
    unsigned GIObjectCnt; // contador de objetos da GI
    unsigned ITObjectCnt; //

    bool bITreceived;

    // ---- pure virtual funcions, user defined on derived class (mandatory)---
    // make tcp connection, user provided
    virtual void connectTCP() = 0;
    // tcp disconnect, user provided
    virtual void disconnectTCP() = 0;
    // read tcp data, user provided
    virtual int readTCP(char *buf, int szmax) = 0;
    // send tcp data, user provided
    virtual void sendTCP(char *data, int sz) = 0;

    // ---- virtual funcions, user defined on derived class (not mandatory)---
    // user point process, user provided. (on one call must be only objects of one type)
    virtual void dataIndication(iec_obj * /*obj*/, int /*numpoints*/){};
    // inform user that ACTCONFIRM of Interrogation was received from slave
    virtual void interrogationActConfIndication(){};
    // inform user that ACTTERM of Interrogation was received from slave
    virtual void interrogationActTermIndication(){};
    // inform user that ACTCONFIRM of IntegralTotals was received from slave
    virtual void integraltotalActConfIndication(){};
    // inform user that ACTTERM of IntegralTotals was received from slave
    virtual void integraltotalActTermIndication(){};
    // inform user of command activation
    virtual void commandActConfIndication(iec_obj * /*obj*/){};
    // inform user of command termination
    virtual void commandActTermIndication(iec_obj * /*obj*/){};
    // user process APDU
    virtual void userprocAPDU(iec_apdu * /* papdu */, int /* sz */){};

    // -------------------------------------------------------------------------
};