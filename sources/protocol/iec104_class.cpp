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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <string>
#include <sstream>

#include "iec104_class.h"

using namespace std;

iec104_class::iec104_class() : t3_testfr(10), t2_supervisory(8), t1_startdtact(6)
{
    strncpy(slaveIP, "", 20);

    Port = 2404;

    msg_supervisory = true;
    seq_order_check = true;
    connectedTCP = false;
    bITreceived = false;

    tout_startdtact = -1;
    tout_supervisory = -1;
    tout_testfr = -1;
    tout_gi = -1;
    VS = 0;
    VR = 0;
    TxOk = false;
    masterAddress = 0;
    slaveAddress = 0;
    GIObjectCnt = 0;
}

void iec104_class::disableSequenceOrderCheck()
{
    seq_order_check = false;
}

int iec104_class::getPortTCP()
{
    return Port;
}

void iec104_class::setPortTCP(unsigned port)
{
    Port = port;
}

void iec104_class::setSecondaryIP(char *ip)
{
    strncpy(slaveIP, ip, 20);
}

char *iec104_class::getSecondaryIP()
{
    return slaveIP;
}

void iec104_class::setSecondaryAddress(int addr)
{
    slaveAddress = addr;
}

int iec104_class::getSecondaryAddress()
{
    return slaveAddress;
}

void iec104_class::setPrimaryAddress(int addr)
{
    masterAddress = addr;
}

int iec104_class::getPrimaryAddress()
{
    return masterAddress;
}

void iec104_class::onConnectTCP()
{
    connectedTCP = true;
    TxOk = false;
    VS = 0;
    VR = 0;
    // mLog.pushMsg("*** TCP CONNECT!");
    mLog.pushMsg("*** TCP ����!");
    sendStartDTACT();
}

void iec104_class::onDisconnectTCP()
{
    connectedTCP = false;
    tout_startdtact = -1;
    tout_supervisory = -1;
    tout_gi = -1;
    TxOk = false;
    // mLog.pushMsg("*** TCP DISCONNECT!");
    mLog.pushMsg("*** TCP �Ͽ�����!");
}

void iec104_class::onTimerSecond()
{
    iec_apdu apdu;
    static uint32_t cnts = 1;

    cnts++;

    if (!(cnts % 5))
        if (!connectedTCP)
            connectTCP();

    if (connectedTCP)
    {
        if (tout_startdtact > 0)
            tout_startdtact--;
        if (tout_startdtact == 0) // timeout of startdtact: retry
            sendStartDTACT();

        if (tout_gi > 0)
        {
            tout_gi--;
            if (tout_gi == 0)
                solicitGI();
        }

        if (msg_supervisory)
        {
            if (tout_supervisory > 0)
                tout_supervisory--;
            if (tout_supervisory > 0)
                tout_supervisory--;

            if (tout_supervisory == 0)
            {
                tout_supervisory = -1;
                sendSupervisory();
            }
        }
    }

    // if connected and no data received, send TESTFRACT
    if (connectedTCP && TxOk)
    {
        if (tout_testfr > 0)
        {
            tout_testfr--;
            if (tout_testfr == 0)
            {
                apdu.start = START;
                apdu.length = 4;
                apdu.NS = TESTFRACT;
                apdu.NR = 0;
                sendTCP((char *)&apdu, 6);
                // mLog.pushMsg("<-- TESTFRACT");
                mLog.pushMsg("<-- ��·����");
            }
        }
    }
}

void iec104_class::solicitGI()
{
    iec_apdu wapdu;

    wapdu.start = START;
    wapdu.length = 0x0E;
    wapdu.NS = VS;
    wapdu.NR = VR;
    wapdu.asduh.type = INTERROGATION;
    wapdu.asduh.num = 1;
    wapdu.asduh.sq = 0;
    wapdu.asduh.cause = ACTIVATION;
    wapdu.asduh.t = 0;
    wapdu.asduh.pn = 0;
    /*original but wrong
    wapdu.asduh.oa = masterAddress;
    wapdu.asduh.ca = 0x00;
    */
    wapdu.asduh.oa = 0x00;
    wapdu.asduh.ca = masterAddress;
    wapdu.dados[0] = 0x00;
    wapdu.dados[1] = 0x00;
    wapdu.dados[2] = 0x00;
    wapdu.dados[3] = 0x14;
    sendTCP((char *)&wapdu, 16);
    VS += 2;
    // mLog.pushMsg( "<-- INTERROGATION " );
    mLog.pushMsg("<-- ���ٻ����� ");
}

void iec104_class::solicitIntegratedTotal()
{
    iec_apdu wapdu;

    wapdu.start = START;
    wapdu.length = 0x0E;
    wapdu.NS = VS;
    wapdu.NR = VR;
    wapdu.asduh.type = INTEGRATEDTOTALS;
    wapdu.asduh.num = 1;
    wapdu.asduh.sq = 0;
    wapdu.asduh.cause = ACTIVATION;
    wapdu.asduh.t = 0;
    wapdu.asduh.pn = 0;
    /*original but wrong��reserve for later checking 2014/7/15
    wapdu.asduh.oa = masterAddress;
    wapdu.asduh.ca = 0x00;
    */
    wapdu.asduh.oa = 0x00;
    wapdu.asduh.ca = masterAddress;
    wapdu.dados[0] = 0x00;
    wapdu.dados[1] = 0x00;
    wapdu.dados[2] = 0x00;
    wapdu.dados[3] = 0x45;
    sendTCP((char *)&wapdu, 16);
    VS += 2;
    // mLog.pushMsg( "<-------- INTEGRAL TOTAL " );
    mLog.pushMsg("<-------- ң���������� ");
}

// �����糧�ô�������Ϊң���������
void iec104_class::ReadIntegratedTotal()
{
    iec_apdu wapdu;

    wapdu.start = START;
    wapdu.length = 0x0E;
    wapdu.NS = VS;
    wapdu.NR = VR;
    wapdu.asduh.type = INTEGRATEDTOTALS;
    wapdu.asduh.num = 1;
    wapdu.asduh.sq = 0;
    wapdu.asduh.cause = ACTIVATION;
    wapdu.asduh.t = 0;
    wapdu.asduh.pn = 0;
    /*original but wrong��reserve for later checking 2014/7/15
    wapdu.asduh.oa = masterAddress;
    wapdu.asduh.ca = 0x00;
    */
    wapdu.asduh.oa = 0x00;
    wapdu.asduh.ca = masterAddress;
    wapdu.dados[0] = 0x00;
    wapdu.dados[1] = 0x00;
    wapdu.dados[2] = 0x00;
    wapdu.dados[3] = 0x05;
    sendTCP((char *)&wapdu, 16);
    VS += 2;
    mLog.pushMsg("<-------- ң�������� ");
}

void iec104_class::confTestCommand()
{
    iec_apdu wapdu;

    wapdu.start = START;
    wapdu.length = 22;
    wapdu.NS = VS;
    wapdu.NR = VR;
    wapdu.asduh.type = C_TS_TA_1;
    wapdu.asduh.num = 1;
    wapdu.asduh.sq = 0;
    wapdu.asduh.cause = ACTCONFIRM;
    wapdu.asduh.t = 0;
    wapdu.asduh.pn = 0;
    wapdu.asduh.oa = masterAddress;
    wapdu.asduh.ca = slaveAddress;
    time_t tm1 = time(NULL);
    tm *agora = localtime(&tm1);

    wapdu.asdu107.ioa16 = 0;
    wapdu.asdu107.ioa8 = 0;
    wapdu.asdu107.tsc = 0;
    wapdu.asdu107.time.year = agora->tm_year % 100;
    wapdu.asdu107.time.month = agora->tm_mon;
    wapdu.asdu107.time.mday = agora->tm_mday;
    wapdu.asdu107.time.hour = agora->tm_hour;
    wapdu.asdu107.time.min = agora->tm_min;
    wapdu.asdu107.time.msec = agora->tm_sec * 1000;

    sendTCP((char *)&wapdu, 22 + 2);
    VS += 2;

    mLog.pushMsg("<-- TEST COMMAND CONF ");
}

void iec104_class::sendStartDTACT()
{
    // send STARTDTACT: enable data transfer
    iec_apdu apdu;
    apdu.start = START;
    apdu.length = 4;
    apdu.NS = STARTDTACT;
    apdu.NR = 0;
    sendTCP((char *)&apdu, 6);
    mLog.pushMsg("<-- STARTDTACT");
    // mLog.pushMsg("<-- STARTDT����");
    tout_startdtact = t1_startdtact;
}

// tcp packet ready to be read from connection with the iec104 slave
void iec104_class::packetReadyTCP()
{
    static bool broken_msg = false;
    static iec_apdu apdu;
    uint8_t *br;
    br = (uint8_t *)&apdu;
    int bytesrec;
    uint8_t byt;
    uint8_t len;

    while (true)
    {

        if (!broken_msg)
        {
            // look for a START
            do
            {
                bytesrec = readTCP((char *)br, 1);
                if (bytesrec == 0)
                    return;
                byt = br[0];
            } while (byt != START);
            bytesrec = readTCP((char *)br + 1, 1); // length of apdu
            if (bytesrec == 0)
                return;
        }

        len = br[1];
        if (len < 4) // apdu length must be >= 4
        {
            broken_msg = false;
            mLog.pushMsg("--> ERROR: INVALID FRAME");
            continue;
        }

        bytesrec = readTCP((char *)br + 2, len); // read the remaining of the apdu
        if (bytesrec == 0)
        {
            mLog.pushMsg("--> Broken apdu");
            broken_msg = true;
            return;
        }

        /*������ַ��ȷҲ�ᱨ����
         if ( apdu.asduh.ca != slaveAddress && len>4 )
           {
           broken_msg=false;
           mLog.pushMsg("--> ASDU WITH UNEXPECTED ORIGIN! Ignoring...");
           // continue;
           }
           */
        broken_msg = false;

        if (mLog.isLogging())
        {
            char buflog[5000];

            sprintf(buflog, "--> %03d: ", (int)len + 2);
            for (int i = 0; i < len + 2 && i < 25; i++) // log up to 25 caracteres
                                                        // for (int i=0; i< len+2; i++) // log all caracteres only when debug
                sprintf(buflog + strlen(buflog), "%02x ", br[i]);
            mLog.pushMsg(buflog);
        }

        userprocAPDU(&apdu, len + 2);
        parseAPDU(&apdu, len + 2);
        break;
    }

    /*
    char buf[5000];
    buf[0]=0;

    sprintf(buf+strlen(buf), "--> ");
    bytesrec=readTCP((char*)br,2); // start and length
    if ( br[0]== START) // valid frames must begin with START
    {
        bytesrec=readTCP((char*)br+2,br[1]); // read the remaining of the frame
        sprintf (buf+strlen(buf), "%03d: ", bytesrec+2);
        int i=0;
        for (; i< bytesrec+2 && i<25 ; i++) // prints up to 25 chars
            sprintf (buf+strlen(buf), "%02x ", br[i]);
        mLog.pushMsg(buf);
        buf[0]=0;
        parseAPDU(&apdu, bytesrec+2);
    }
    else
    {
        mLog.pushMsg("--> ERROR: INVALID FRAME");
        bytesrec=readTCP((char*)br,sizeof(apdu)); // consider garbage the remaining data from the frame
    }
    */
}

void iec104_class::parseAPDU(iec_apdu *papdu, int sz, bool accountandrespond)
{
    iec_apdu wapdu; // buffer to assemble apdu to send
    string qs, qsa;
    stringstream oss;
    uint16_t VR_NEW;

    if (papdu->start != START)
    { // invalid frame
        mLog.pushMsg("--> ERROR: NO START IN FRAME");
        return;
    }

    /*always cause failure when slave is poorly implemented
    if ( papdu->asduh.ca != slaveAddress && sz>6)
    { // invalid frame
        mLog.pushMsg("--> ASDU WITH UNEXPECTED ORIGIN! Ignoring...");
        return;
    }
    */

    if (sz == 6)
    { // Control messages
        if (accountandrespond)
            switch (papdu->NS)
            {
            case STARTDTACT:
                mLog.pushMsg("<-- STARTDTACT");
                // mLog.pushMsg("<-- STARTDT����");
                wapdu.start = START;
                wapdu.length = 4;
                wapdu.NS = STARTDTCON;
                wapdu.NR = 0;
                sendTCP((char *)&wapdu, 6);
                mLog.pushMsg("    STARTDTCON");
                // mLog.pushMsg("    STARTDTȷ��");
                break;

            case TESTFRACT:
                mLog.pushMsg("<-- TESTFRAACT");
                // mLog.pushMsg("<-- ��·��������");
                wapdu.start = START;
                wapdu.length = 4;
                wapdu.NS = TESTFRCON;
                wapdu.NR = 0;
                sendTCP((char *)&wapdu, 6);
                // mLog.pushMsg("   TESTFRCON");
                // mLog.pushMsg("   ��·������Ӧ");
                break;

            case STARTDTCON:
                mLog.pushMsg("--> STARTDTCON");
                // mLog.pushMsg("--> STARTDTȷ��");
                tout_startdtact = -1; // flag confirmation of STARTDT, not to timeout
                TxOk = true;
                tout_gi = 10;
                break;

            case STOPDTACT:
                mLog.pushMsg("-->  STOPDTACT");
                // mLog.pushMsg("-->  STOPDT����");
                //  only slave responds
                break;

            case STOPDTCON:
                mLog.pushMsg("--> STOPDTCON");
                // mLog.pushMsg("--> STOPDTȷ��");
                //  do what?
                break;

            case TESTFRCON:
                mLog.pushMsg("--> TESTFRCON");
                // mLog.pushMsg("--> TESTFRȷ��");
                //  do what?
                break;

            case SUPERVISORY:
                // mLog.pushMsg("--> SUPERVISORY");
                mLog.pushMsg("--> SUPERVISORY");
                // do what?
                break;

            default: // error
                mLog.pushMsg("    ERROR: UNKNOWN CONTROL MESSAGE");
                break;
            }
    }
    else
    { // data message

        if (accountandrespond)
        {
            VR_NEW = (papdu->NS & 0xFFFE);

            if (VR_NEW != VR)
            {
                // sequence error, must close and reopen connection
                mLog.pushMsg("*** SEQUENCE ERROR! **************************");
                if (seq_order_check)
                {
                    disconnectTCP();
                    return;
                }
            }

            VR = VR_NEW + 2;
        }

        oss.str("");
        oss << "    CA "
            << (unsigned)papdu->asduh.ca
            << " TYPE "
            << (unsigned)papdu->asduh.type
            << " CAUSE "
            << (int)papdu->asduh.cause
            << " SQ "
            << (unsigned)papdu->asduh.sq
            << " NUM "
            << (unsigned)papdu->asduh.num;
        mLog.pushMsg((char *)oss.str().c_str());

        switch (papdu->asduh.type)
        {
        case M_SP_NA_1: // 1: DIGITAL SINGLE
        {
            uint32_t addr24 = 0;
            iec_type1 *pobj;
            iec_obj *piecarr = new iec_obj[papdu->asduh.num];
            if (papdu->asduh.cause == 20)
                GIObjectCnt += papdu->asduh.num;

            for (int i = 0; i < papdu->asduh.num; i++)
            {
                if (papdu->asduh.sq)
                {
                    pobj = &papdu->sq1.obj[i];
                    if (i == 0)
                        addr24 = papdu->sq1.ioa16 + ((unsigned)papdu->sq1.ioa8 << 16);
                    else
                        addr24++;
                }
                else
                {
                    pobj = &papdu->nsq1[i].obj;
                    addr24 = papdu->nsq1[i].ioa16 + ((unsigned)papdu->nsq1[i].ioa8 << 16);
                }

                piecarr[i].address = addr24;
                piecarr[i].ca = papdu->asduh.ca;
                piecarr[i].cause = papdu->asduh.cause;
                piecarr[i].pn = papdu->asduh.pn;
                piecarr[i].type = papdu->asduh.type;
                piecarr[i].value = pobj->sp;
                piecarr[i].sp = pobj->sp;
                piecarr[i].bl = pobj->bl;
                piecarr[i].nt = pobj->nt;
                piecarr[i].sb = pobj->sb;
                piecarr[i].iv = pobj->iv;
            }
            dataIndication(piecarr, papdu->asduh.num);
            delete[] piecarr;
        }
        break;
        case M_DP_NA_1: // 3: DIGITAL DOUBLE
        {
            uint32_t addr24 = 0;
            iec_type3 *pobj;
            iec_obj *piecarr = new iec_obj[papdu->asduh.num];
            if (papdu->asduh.cause == 20)
                GIObjectCnt += papdu->asduh.num;

            for (int i = 0; i < papdu->asduh.num; i++)
            {
                if (papdu->asduh.sq)
                {
                    pobj = &papdu->sq3.obj[i];
                    if (i == 0)
                        addr24 = papdu->sq3.ioa16 + ((unsigned)papdu->sq3.ioa8 << 16);
                    else
                        addr24++;
                }
                else
                {
                    pobj = &papdu->nsq3[i].obj;
                    addr24 = papdu->nsq3[i].ioa16 + ((unsigned)papdu->nsq3[i].ioa8 << 16);
                }

                piecarr[i].address = addr24;
                piecarr[i].ca = papdu->asduh.ca;
                piecarr[i].cause = papdu->asduh.cause;
                piecarr[i].pn = papdu->asduh.pn;
                piecarr[i].type = papdu->asduh.type;
                piecarr[i].value = pobj->dp;
                piecarr[i].dp = pobj->dp;
                piecarr[i].bl = pobj->bl;
                piecarr[i].nt = pobj->nt;
                piecarr[i].sb = pobj->sb;
                piecarr[i].iv = pobj->iv;
            }
            dataIndication(piecarr, papdu->asduh.num);
            delete[] piecarr;
        }
        break;
        case M_ST_NA_1: // 5: step position
        {
            uint32_t addr24 = 0;
            iec_type5 *pobj;
            iec_obj *piecarr = new iec_obj[papdu->asduh.num];
            if (papdu->asduh.cause == 20)
                GIObjectCnt += papdu->asduh.num;

            for (int i = 0; i < papdu->asduh.num; i++)
            {
                if (papdu->asduh.sq)
                {
                    pobj = &papdu->sq5.obj[i];
                    if (i == 0)
                        addr24 = papdu->sq5.ioa16 + ((unsigned)papdu->sq5.ioa8 << 16);
                    else
                        addr24++;
                }
                else
                {
                    pobj = &papdu->nsq5[i].obj;
                    addr24 = papdu->nsq5[i].ioa16 + ((unsigned)papdu->nsq5[i].ioa8 << 16);
                }

                piecarr[i].address = addr24;
                piecarr[i].ca = papdu->asduh.ca;
                piecarr[i].cause = papdu->asduh.cause;
                piecarr[i].pn = papdu->asduh.pn;
                piecarr[i].type = papdu->asduh.type;
                piecarr[i].value = pobj->mv;
                piecarr[i].t = pobj->t;
                piecarr[i].ov = pobj->ov;
                piecarr[i].bl = pobj->bl;
                piecarr[i].nt = pobj->nt;
                piecarr[i].sb = pobj->sb;
                piecarr[i].iv = pobj->iv;
            }
            dataIndication(piecarr, papdu->asduh.num);
            delete[] piecarr;
        }
        break;
        case M_ME_NA_1: // 9: ANALOGIC NORMALIZED
        {
            uint32_t addr24 = 0;
            iec_type9 *pobj;
            iec_obj *piecarr = new iec_obj[papdu->asduh.num];
            if (papdu->asduh.cause == 20)
                GIObjectCnt += papdu->asduh.num;

            for (int i = 0; i < papdu->asduh.num; i++)
            {
                if (papdu->asduh.sq)
                {
                    pobj = &papdu->sq9.obj[i];
                    if (i == 0)
                        addr24 = papdu->sq9.ioa16 + ((unsigned)papdu->sq9.ioa8 << 16);
                    else
                        addr24++;
                }
                else
                {
                    pobj = &papdu->nsq9[i].obj;
                    addr24 = papdu->nsq9[i].ioa16 + ((unsigned)papdu->nsq9[i].ioa8 << 16);
                }

                piecarr[i].address = addr24;
                piecarr[i].ca = papdu->asduh.ca;
                piecarr[i].cause = papdu->asduh.cause;
                piecarr[i].pn = papdu->asduh.pn;
                piecarr[i].type = papdu->asduh.type;
                piecarr[i].value = pobj->mv;
                piecarr[i].ov = pobj->ov;
                piecarr[i].bl = pobj->bl;
                piecarr[i].nt = pobj->nt;
                piecarr[i].sb = pobj->sb;
                piecarr[i].iv = pobj->iv;
            }
            dataIndication(piecarr, papdu->asduh.num);
            delete[] piecarr;
        }
        break;
        case M_ME_NB_1: // 11: ANALOGIC CONVERTED
        {
            uint32_t addr24 = 0;
            iec_type11 *pobj;
            iec_obj *piecarr = new iec_obj[papdu->asduh.num];
            if (papdu->asduh.cause == 20)
                GIObjectCnt += papdu->asduh.num;

            for (int i = 0; i < papdu->asduh.num; i++)
            {
                if (papdu->asduh.sq)
                {
                    pobj = &papdu->sq11.obj[i];
                    if (i == 0)
                        addr24 = papdu->sq11.ioa16 + ((unsigned)papdu->sq11.ioa8 << 16);
                    else
                        addr24++;
                }
                else
                {
                    pobj = &papdu->nsq11[i].obj;
                    addr24 = papdu->nsq11[i].ioa16 + ((unsigned)papdu->nsq11[i].ioa8 << 16);
                }

                piecarr[i].address = addr24;
                piecarr[i].ca = papdu->asduh.ca;
                piecarr[i].cause = papdu->asduh.cause;
                piecarr[i].pn = papdu->asduh.pn;
                piecarr[i].type = papdu->asduh.type;
                piecarr[i].value = pobj->mv;
                piecarr[i].ov = pobj->ov;
                piecarr[i].bl = pobj->bl;
                piecarr[i].nt = pobj->nt;
                piecarr[i].sb = pobj->sb;
                piecarr[i].iv = pobj->iv;
            }
            dataIndication(piecarr, papdu->asduh.num);
            delete[] piecarr;
        }
        break;
        case M_ME_NC_1: // 13: ANALOGIC FLOATING POINT
        {
            uint32_t addr24 = 0;
            iec_type13 *pobj;
            iec_obj *piecarr = new iec_obj[papdu->asduh.num];
            if (papdu->asduh.cause == 20)
                GIObjectCnt += papdu->asduh.num;

            for (int i = 0; i < papdu->asduh.num; i++)
            {
                if (papdu->asduh.sq)
                {
                    pobj = &papdu->sq13.obj[i];
                    if (i == 0)
                        addr24 = papdu->sq13.ioa16 + ((unsigned)papdu->sq13.ioa8 << 16);
                    else
                        addr24++;
                }
                else
                {
                    pobj = &papdu->nsq13[i].obj;
                    addr24 = papdu->nsq13[i].ioa16 + ((unsigned)papdu->nsq13[i].ioa8 << 16);
                }

                // reverse((char*)&pobj->mv, sizeof(pobj->mv));

                piecarr[i].address = addr24;
                piecarr[i].ca = papdu->asduh.ca;
                piecarr[i].cause = papdu->asduh.cause;
                piecarr[i].pn = papdu->asduh.pn;
                piecarr[i].type = papdu->asduh.type;
                piecarr[i].value = pobj->mv;
                piecarr[i].ov = pobj->ov;
                piecarr[i].bl = pobj->bl;
                piecarr[i].nt = pobj->nt;
                piecarr[i].sb = pobj->sb;
                piecarr[i].iv = pobj->iv;
            }
            dataIndication(piecarr, papdu->asduh.num);
            delete[] piecarr;
        }
        break;
        case M_IT_NA_1: // 15
        {
            uint32_t addr24 = 0;
            iec_type15 *pobj;
            iec_obj *piecarr = new iec_obj[papdu->asduh.num];
            if (papdu->asduh.cause == 37)
                ITObjectCnt += papdu->asduh.num;

            for (int i = 0; i < papdu->asduh.num; i++)
            {
                if (papdu->asduh.sq)
                {
                    pobj = &papdu->sq15.obj[i];
                    if (i == 0)
                        addr24 = papdu->sq15.ioa16 + ((unsigned)papdu->sq15.ioa8 << 16);
                    else
                        addr24++;
                }
                else
                {
                    pobj = &papdu->nsq15[i].obj;
                    addr24 = papdu->nsq15[i].ioa16 + ((unsigned)papdu->nsq15[i].ioa8 << 16);
                }
                piecarr[i].address = addr24;
                piecarr[i].ca = papdu->asduh.ca;
                piecarr[i].cause = papdu->asduh.cause;
                piecarr[i].pn = papdu->asduh.pn;
                piecarr[i].type = papdu->asduh.type;
                piecarr[i].value = pobj->mv;
            }
            dataIndication(piecarr, papdu->asduh.num);
            delete[] piecarr;
        }
        break;
        case M_ME_ND_1: // 21
        {
            uint32_t addr24 = 0;
            iec_type21 *pobj;
            iec_obj *piecarr = new iec_obj[papdu->asduh.num];
            if (papdu->asduh.cause == 20)
                GIObjectCnt += papdu->asduh.num;

            for (int i = 0; i < papdu->asduh.num; i++)
            {
                if (papdu->asduh.sq)
                {
                    pobj = &papdu->sq21.obj[i];
                    if (i == 0)
                        addr24 = papdu->sq21.ioa16 + ((unsigned)papdu->sq21.ioa8 << 16);
                    else
                        addr24++;
                }
                else
                {
                    pobj = &papdu->nsq21[i].obj;
                    addr24 = papdu->nsq21[i].ioa16 + ((unsigned)papdu->nsq21[i].ioa8 << 16);
                }

                piecarr[i].address = addr24;
                piecarr[i].ca = papdu->asduh.ca;
                piecarr[i].cause = papdu->asduh.cause;
                piecarr[i].pn = papdu->asduh.pn;
                piecarr[i].type = papdu->asduh.type;
                piecarr[i].value = pobj->mv;
            }
            dataIndication(piecarr, papdu->asduh.num);
            delete[] piecarr;
        }
        break;
        case M_SP_TB_1: // 30:  DIGITAL SINGLE WITH LONG TIME TAG
        {
            uint32_t addr24 = 0;
            iec_type30 *pobj;
            iec_obj *piecarr = new iec_obj[papdu->asduh.num];
            if (papdu->asduh.cause == 20)
                GIObjectCnt += papdu->asduh.num;

            for (int i = 0; i < papdu->asduh.num; i++)
            {
                if (papdu->asduh.sq)
                {
                    pobj = &papdu->sq30.obj[i];
                    if (i == 0)
                        addr24 = papdu->sq30.ioa16 + ((unsigned)papdu->sq30.ioa8 << 16);
                    else
                        addr24++;
                }
                else
                {
                    pobj = &papdu->nsq30[i].obj;
                    addr24 = papdu->nsq30[i].ioa16 + ((unsigned)papdu->nsq30[i].ioa8 << 16);
                }

                piecarr[i].address = addr24;
                piecarr[i].ca = papdu->asduh.ca;
                piecarr[i].cause = papdu->asduh.cause;
                piecarr[i].pn = papdu->asduh.pn;
                piecarr[i].type = papdu->asduh.type;
                piecarr[i].value = pobj->sp;
                piecarr[i].sp = pobj->sp;
                piecarr[i].bl = pobj->bl;
                piecarr[i].nt = pobj->nt;
                piecarr[i].sb = pobj->sb;
                piecarr[i].iv = pobj->iv;
                piecarr[i].timetag.mday = pobj->time.mday;
                piecarr[i].timetag.month = pobj->time.month;
                piecarr[i].timetag.year = pobj->time.year;
                piecarr[i].timetag.hour = pobj->time.hour;
                piecarr[i].timetag.min = pobj->time.min;
                piecarr[i].timetag.msec = pobj->time.msec;
                piecarr[i].timetag.iv = pobj->time.iv;
            }
            dataIndication(piecarr, papdu->asduh.num);
            delete[] piecarr;
        }
        break;
        case M_DP_TB_1: // 31: DIGITAL DOUBLE WITH LONG TIME TAG
        {
            uint32_t addr24 = 0;
            iec_type31 *pobj;
            iec_obj *piecarr = new iec_obj[papdu->asduh.num];
            if (papdu->asduh.cause == 20)
                GIObjectCnt += papdu->asduh.num;

            for (int i = 0; i < papdu->asduh.num; i++)
            {
                if (papdu->asduh.sq)
                {
                    pobj = &papdu->sq31.obj[i];
                    if (i == 0)
                        addr24 = papdu->sq31.ioa16 + ((unsigned)papdu->sq31.ioa8 << 16);
                    else
                        addr24++;
                }
                else
                {
                    pobj = &papdu->nsq31[i].obj;
                    addr24 = papdu->nsq31[i].ioa16 + ((unsigned)papdu->nsq31[i].ioa8 << 16);
                }

                piecarr[i].address = addr24;
                piecarr[i].ca = papdu->asduh.ca;
                piecarr[i].cause = papdu->asduh.cause;
                piecarr[i].pn = papdu->asduh.pn;
                piecarr[i].type = papdu->asduh.type;
                piecarr[i].value = pobj->dp;
                piecarr[i].dp = pobj->dp;
                piecarr[i].bl = pobj->bl;
                piecarr[i].nt = pobj->nt;
                piecarr[i].sb = pobj->sb;
                piecarr[i].iv = pobj->iv;
                piecarr[i].timetag.mday = pobj->time.mday;
                piecarr[i].timetag.month = pobj->time.month;
                piecarr[i].timetag.year = pobj->time.year;
                piecarr[i].timetag.hour = pobj->time.hour;
                piecarr[i].timetag.min = pobj->time.min;
                piecarr[i].timetag.msec = pobj->time.msec;
                piecarr[i].timetag.iv = pobj->time.iv;
            }
            dataIndication(piecarr, papdu->asduh.num);
            delete[] piecarr;
        }
        break;
        case M_ST_TB_1: // 32: TAP WITH TIME TAG
        {
            uint32_t addr24 = 0;
            iec_type32 *pobj;
            iec_obj *piecarr = new iec_obj[papdu->asduh.num];
            if (papdu->asduh.cause == 20)
                GIObjectCnt += papdu->asduh.num;

            for (int i = 0; i < papdu->asduh.num; i++)
            {
                if (papdu->asduh.sq)
                {
                    pobj = &papdu->sq32.obj[i];
                    if (i == 0)
                        addr24 = papdu->sq32.ioa16 + ((unsigned)papdu->sq32.ioa8 << 16);
                    else
                        addr24++;
                }
                else
                {
                    pobj = &papdu->nsq32[i].obj;
                    addr24 = papdu->nsq32[i].ioa16 + ((unsigned)papdu->nsq32[i].ioa8 << 16);
                }

                piecarr[i].address = addr24;
                piecarr[i].ca = papdu->asduh.ca;
                piecarr[i].cause = papdu->asduh.cause;
                piecarr[i].pn = papdu->asduh.pn;
                piecarr[i].type = papdu->asduh.type;
                piecarr[i].value = pobj->mv;
                piecarr[i].t = pobj->t;
                piecarr[i].ov = pobj->ov;
                piecarr[i].bl = pobj->bl;
                piecarr[i].nt = pobj->nt;
                piecarr[i].sb = pobj->sb;
                piecarr[i].iv = pobj->iv;
                piecarr[i].timetag.mday = pobj->time.mday;
                piecarr[i].timetag.month = pobj->time.month;
                piecarr[i].timetag.year = pobj->time.year;
                piecarr[i].timetag.hour = pobj->time.hour;
                piecarr[i].timetag.min = pobj->time.min;
                piecarr[i].timetag.msec = pobj->time.msec;
                piecarr[i].timetag.iv = pobj->time.iv;
            }
            dataIndication(piecarr, papdu->asduh.num);
            delete[] piecarr;
        }
        break;
        case M_BO_NA_1: // 7
            mLog.pushMsg("!!! TYPE NOT IMPLEMENTED");
            break;
        case M_BO_TB_1: // 33
            mLog.pushMsg("!!! TYPE NOT IMPLEMENTED");
            break;
        case M_ME_TD_1: // 34 MEASURED VALUE, NORMALIZED WITH TIME TAG
        {
            uint32_t addr24 = 0;
            iec_type34 *pobj;
            iec_obj *piecarr = new iec_obj[papdu->asduh.num];
            if (papdu->asduh.cause == 20)
                GIObjectCnt += papdu->asduh.num;

            for (int i = 0; i < papdu->asduh.num; i++)
            {
                if (papdu->asduh.sq)
                {
                    pobj = &papdu->sq34.obj[i];
                    if (i == 0)
                        addr24 = papdu->sq34.ioa16 + ((unsigned)papdu->sq34.ioa8 << 16);
                    else
                        addr24++;
                }
                else
                {
                    pobj = &papdu->nsq34[i].obj;
                    addr24 = papdu->nsq34[i].ioa16 + ((unsigned)papdu->nsq34[i].ioa8 << 16);
                }

                piecarr[i].address = addr24;
                piecarr[i].ca = papdu->asduh.ca;
                piecarr[i].cause = papdu->asduh.cause;
                piecarr[i].pn = papdu->asduh.pn;
                piecarr[i].type = papdu->asduh.type;
                piecarr[i].value = pobj->mv;
                piecarr[i].ov = pobj->ov;
                piecarr[i].bl = pobj->bl;
                piecarr[i].nt = pobj->nt;
                piecarr[i].sb = pobj->sb;
                piecarr[i].iv = pobj->iv;
                piecarr[i].timetag.mday = pobj->time.mday;
                piecarr[i].timetag.month = pobj->time.month;
                piecarr[i].timetag.year = pobj->time.year;
                piecarr[i].timetag.hour = pobj->time.hour;
                piecarr[i].timetag.min = pobj->time.min;
                piecarr[i].timetag.msec = pobj->time.msec;
                piecarr[i].timetag.iv = pobj->time.iv;
            }
            dataIndication(piecarr, papdu->asduh.num);
            delete[] piecarr;
        }
        break;
        case M_ME_TE_1: // 35 MEASURED VALUE, SCALED WITH TIME TAG
        {
            uint32_t addr24 = 0;
            iec_type35 *pobj;
            iec_obj *piecarr = new iec_obj[papdu->asduh.num];
            if (papdu->asduh.cause == 20)
                GIObjectCnt += papdu->asduh.num;

            for (int i = 0; i < papdu->asduh.num; i++)
            {
                if (papdu->asduh.sq)
                {
                    pobj = &papdu->sq35.obj[i];
                    if (i == 0)
                        addr24 = papdu->sq35.ioa16 + ((unsigned)papdu->sq35.ioa8 << 16);
                    else
                        addr24++;
                }
                else
                {
                    pobj = &papdu->nsq35[i].obj;
                    addr24 = papdu->nsq35[i].ioa16 + ((unsigned)papdu->nsq35[i].ioa8 << 16);
                }

                piecarr[i].address = addr24;
                piecarr[i].ca = papdu->asduh.ca;
                piecarr[i].cause = papdu->asduh.cause;
                piecarr[i].pn = papdu->asduh.pn;
                piecarr[i].type = papdu->asduh.type;
                piecarr[i].value = pobj->mv;
                piecarr[i].ov = pobj->ov;
                piecarr[i].bl = pobj->bl;
                piecarr[i].nt = pobj->nt;
                piecarr[i].sb = pobj->sb;
                piecarr[i].iv = pobj->iv;
                piecarr[i].timetag.mday = pobj->time.mday;
                piecarr[i].timetag.month = pobj->time.month;
                piecarr[i].timetag.year = pobj->time.year;
                piecarr[i].timetag.hour = pobj->time.hour;
                piecarr[i].timetag.min = pobj->time.min;
                piecarr[i].timetag.msec = pobj->time.msec;
                piecarr[i].timetag.iv = pobj->time.iv;
            }
            dataIndication(piecarr, papdu->asduh.num);
            delete[] piecarr;
        }
        break;
        case M_ME_TF_1: // 36 MEASURED VALUE, FLOATING POINT WITH TIME TAG
        {
            uint32_t addr24 = 0;
            iec_type36 *pobj;
            iec_obj *piecarr = new iec_obj[papdu->asduh.num];
            if (papdu->asduh.cause == 20)
                GIObjectCnt += papdu->asduh.num;

            for (int i = 0; i < papdu->asduh.num; i++)
            {
                if (papdu->asduh.sq)
                {
                    pobj = &papdu->sq36.obj[i];
                    if (i == 0)
                        addr24 = papdu->sq36.ioa16 + ((unsigned)papdu->sq36.ioa8 << 16);
                    else
                        addr24++;
                }
                else
                {
                    pobj = &papdu->nsq36[i].obj;
                    addr24 = papdu->nsq36[i].ioa16 + ((unsigned)papdu->nsq36[i].ioa8 << 16);
                }

                piecarr[i].address = addr24;
                piecarr[i].ca = papdu->asduh.ca;
                piecarr[i].cause = papdu->asduh.cause;
                piecarr[i].pn = papdu->asduh.pn;
                piecarr[i].type = papdu->asduh.type;
                piecarr[i].value = pobj->mv;
                piecarr[i].ov = pobj->ov;
                piecarr[i].bl = pobj->bl;
                piecarr[i].nt = pobj->nt;
                piecarr[i].sb = pobj->sb;
                piecarr[i].iv = pobj->iv;
                piecarr[i].timetag.mday = pobj->time.mday;
                piecarr[i].timetag.month = pobj->time.month;
                piecarr[i].timetag.year = pobj->time.year;
                piecarr[i].timetag.hour = pobj->time.hour;
                piecarr[i].timetag.min = pobj->time.min;
                piecarr[i].timetag.msec = pobj->time.msec;
                piecarr[i].timetag.iv = pobj->time.iv;
            }
            dataIndication(piecarr, papdu->asduh.num);
            delete[] piecarr;
        }
        break;
        case M_IT_TB_1: // 37:��ʱ��ĵ����ۻ���
        {
            uint32_t addr24 = 0;
            iec_type37 *pobj;
            iec_obj *piecarr = new iec_obj[papdu->asduh.num];
            if (papdu->asduh.cause == 37)
                ITObjectCnt += papdu->asduh.num;

            for (int i = 0; i < papdu->asduh.num; i++)
            {
                if (papdu->asduh.sq)
                {
                    pobj = &papdu->sq37.obj[i];
                    if (i == 0)
                        addr24 = papdu->sq37.ioa16 + ((unsigned)papdu->sq37.ioa8 << 16);
                    else
                        addr24++;
                }
                else
                {
                    pobj = &papdu->nsq37[i].obj;
                    addr24 = papdu->nsq37[i].ioa16 + ((unsigned)papdu->nsq37[i].ioa8 << 16);
                }

                piecarr[i].address = addr24;
                piecarr[i].ca = papdu->asduh.ca;
                piecarr[i].cause = papdu->asduh.cause;
                piecarr[i].pn = papdu->asduh.pn;
                piecarr[i].type = papdu->asduh.type;

                // if (pobj->np)
                //{
                piecarr[i].value = pobj->bcr; // ��������
                //}else
                //{
                //	piecarr[i].value=pobj->bcr;
                //}

                piecarr[i].iv = pobj->iv;

                piecarr[i].timetag.mday = pobj->time.mday;
                piecarr[i].timetag.month = pobj->time.month;
                piecarr[i].timetag.year = pobj->time.year;
                piecarr[i].timetag.hour = pobj->time.hour;
                piecarr[i].timetag.min = pobj->time.min;
                piecarr[i].timetag.msec = pobj->time.msec;
                piecarr[i].timetag.iv = pobj->time.iv;
            }
            dataIndication(piecarr, papdu->asduh.num);
            delete[] piecarr;
        }
        break;
        case C_SC_NA_1: // SINGLE COMMAND
        {
            iec_type45 *pobj;
            pobj = &papdu->nsq45.obj;

            oss.str("");
            oss << "    ";
            if (papdu->asduh.cause == ACTCONFIRM)
                oss << "ACTIVATION CONFIRMATION ";
            else if (papdu->asduh.cause == ACTTERM)
                oss << "ACTIVATION TERMINATION ";
            if (papdu->asduh.pn == POSITIVE)
                oss << "POSITIVE ";
            else
                oss << "NEGATIVE ";
            oss << "SINGLE COMMAND ADDRESS "
                << (unsigned)papdu->nsq45.ioa16 + ((unsigned)papdu->nsq45.ioa8 << 16)
                << " SCS "
                << (unsigned)pobj->scs
                << " QU "
                << (int)pobj->qu
                << " SE "
                << (unsigned)pobj->se;
            mLog.pushMsg((char *)oss.str().c_str());

            // send indication to user
            iec_obj iobj;
            iobj.address = papdu->nsq45.ioa16 + ((unsigned)papdu->nsq45.ioa8 << 16);
            iobj.cause = papdu->asduh.cause;
            iobj.pn = papdu->asduh.pn;
            iobj.type = papdu->asduh.type;
            iobj.scs = pobj->scs;
            iobj.qu = pobj->qu;
            iobj.se = pobj->se;
            if (papdu->asduh.cause == ACTCONFIRM)
                commandActConfIndication(&iobj);
            else if (papdu->asduh.cause == ACTTERM)
                commandActTermIndication(&iobj);
        }
        break;
        case C_DC_NA_1: // DOUBLE COMMAND
        {
            iec_type46 *pobj;
            pobj = &papdu->nsq46.obj;

            oss.str("");
            oss << "    ";
            if (papdu->asduh.cause == ACTCONFIRM)
                oss << "ACTIVATION CONFIRMATION ";
            else if (papdu->asduh.cause == ACTTERM)
                oss << "ACTIVATION TERMINATION ";
            if (papdu->asduh.pn == POSITIVE)
                oss << "POSITIVE ";
            else
                oss << "NEGATIVE ";
            oss << "DOUBLE COMMAND ADDRESS "
                << (unsigned)papdu->nsq46.ioa16 + ((unsigned)papdu->nsq46.ioa8 << 16)
                << " DCS "
                << (unsigned)pobj->dcs
                << " QU "
                << (int)pobj->qu
                << " SE "
                << (unsigned)pobj->se;
            mLog.pushMsg((char *)oss.str().c_str());

            // send indication to user
            iec_obj iobj;
            iobj.address = papdu->nsq46.ioa16 + ((unsigned)papdu->nsq46.ioa8 << 16);
            iobj.cause = papdu->asduh.cause;
            iobj.pn = papdu->asduh.pn;
            iobj.type = papdu->asduh.type;
            iobj.dcs = pobj->dcs;
            iobj.qu = pobj->qu;
            iobj.se = pobj->se;
            if (papdu->asduh.cause == ACTCONFIRM)
                commandActConfIndication(&iobj);
            else if (papdu->asduh.cause == ACTTERM)
                commandActTermIndication(&iobj);
        }
        break;
        case C_RC_NA_1: // REG.STEP COMMAND
        {
            iec_type47 *pobj;
            pobj = &papdu->nsq47.obj;

            oss.str("");
            oss << "    ";
            if (papdu->asduh.cause == ACTCONFIRM)
                oss << "ACTIVATION CONFIRMATION ";
            else if (papdu->asduh.cause == ACTTERM)
                oss << "ACTIVATION TERMINATION ";
            if (papdu->asduh.pn == POSITIVE)
                oss << "POSITIVE ";
            else
                oss << "NEGATIVE ";
            oss << "STEP REG. COMMAND ADDRESS "
                << (unsigned)papdu->nsq47.ioa16 + ((unsigned)papdu->nsq47.ioa8 << 16)
                << " RCS "
                << (unsigned)pobj->rcs
                << " QU "
                << (int)pobj->qu
                << " SE "
                << (unsigned)pobj->se;
            mLog.pushMsg((char *)oss.str().c_str());
            // send indication to user
            iec_obj iobj;
            iobj.address = papdu->nsq47.ioa16 + ((unsigned)papdu->nsq47.ioa8 << 16);
            iobj.cause = papdu->asduh.cause;
            iobj.pn = papdu->asduh.pn;
            iobj.type = papdu->asduh.type;
            iobj.rcs = pobj->rcs;
            iobj.qu = pobj->qu;
            iobj.se = pobj->se;
            if (papdu->asduh.cause == ACTCONFIRM)
                commandActConfIndication(&iobj);
            else if (papdu->asduh.cause == ACTTERM)
                commandActTermIndication(&iobj);
        }
        break;

        case C_SC_TA_1: // SINGLE COMMAND WITH TIME
        {
            iec_type58 *pobj;
            pobj = &papdu->nsq58.obj;

            oss.str("");
            oss << "    ";
            if (papdu->asduh.cause == ACTCONFIRM)
                oss << "ACTIVATION CONFIRMATION ";
            else if (papdu->asduh.cause == ACTTERM)
                oss << "ACTIVATION TERMINATION ";
            if (papdu->asduh.pn == POSITIVE)
                oss << "POSITIVE ";
            else
                oss << "NEGATIVE ";
            oss << "SINGLE COMMAND ADDRESS "
                << (unsigned)papdu->nsq58.ioa16 + ((unsigned)papdu->nsq58.ioa8 << 16)
                << " SCS "
                << (unsigned)pobj->scs
                << " QU "
                << (int)pobj->qu
                << " SE "
                << (unsigned)pobj->se;
            mLog.pushMsg((char *)oss.str().c_str());

            // send indication to user
            iec_obj iobj;
            iobj.address = papdu->nsq58.ioa16 + ((unsigned)papdu->nsq58.ioa8 << 16);
            iobj.cause = papdu->asduh.cause;
            iobj.pn = papdu->asduh.pn;
            iobj.type = papdu->asduh.type;
            iobj.scs = pobj->scs;
            iobj.qu = pobj->qu;
            iobj.se = pobj->se;
            if (papdu->asduh.cause == ACTCONFIRM)
                commandActConfIndication(&iobj);
            else if (papdu->asduh.cause == ACTTERM)
                commandActTermIndication(&iobj);
        }
        break;
        case C_DC_TA_1: // DOUBLE COMMAND WITH TIME
        {
            iec_type59 *pobj;
            pobj = &papdu->nsq59.obj;

            oss.str("");
            oss << "    ";
            if (papdu->asduh.cause == ACTCONFIRM)
                oss << "ACTIVATION CONFIRMATION ";
            else if (papdu->asduh.cause == ACTTERM)
                oss << "ACTIVATION TERMINATION ";
            if (papdu->asduh.pn == POSITIVE)
                oss << "POSITIVE ";
            else
                oss << "NEGATIVE ";
            oss << "DOUBLE COMMAND ADDRESS "
                << (unsigned)papdu->nsq59.ioa16 + ((unsigned)papdu->nsq59.ioa8 << 16)
                << " DCS "
                << (unsigned)pobj->dcs
                << " QU "
                << (int)pobj->qu
                << " SE "
                << (unsigned)pobj->se;
            mLog.pushMsg((char *)oss.str().c_str());

            // send indication to user
            iec_obj iobj;
            iobj.address = papdu->nsq59.ioa16 + ((unsigned)papdu->nsq59.ioa8 << 16);
            iobj.cause = papdu->asduh.cause;
            iobj.pn = papdu->asduh.pn;
            iobj.type = papdu->asduh.type;
            iobj.dcs = pobj->dcs;
            iobj.qu = pobj->qu;
            iobj.se = pobj->se;
            if (papdu->asduh.cause == ACTCONFIRM)
                commandActConfIndication(&iobj);
            else if (papdu->asduh.cause == ACTTERM)
                commandActTermIndication(&iobj);
        }
        break;
        case C_RC_TA_1: // REG. STEP COMMAND WITH TIME
        {
            iec_type60 *pobj;
            pobj = &papdu->nsq60.obj;

            oss.str("");
            oss << "    ";
            if (papdu->asduh.cause == ACTCONFIRM)
                oss << "ACTIVATION CONFIRMATION ";
            else if (papdu->asduh.cause == ACTTERM)
                oss << "ACTIVATION TERMINATION ";
            if (papdu->asduh.pn == POSITIVE)
                oss << "POSITIVE ";
            else
                oss << "NEGATIVE ";
            oss << "STEP REG. COMMAND ADDRESS "
                << (unsigned)papdu->nsq60.ioa16 + ((unsigned)papdu->nsq60.ioa8 << 16)
                << " RCS "
                << (unsigned)pobj->rcs
                << " QU "
                << (int)pobj->qu
                << " SE "
                << (unsigned)pobj->se;
            mLog.pushMsg((char *)oss.str().c_str());
            // send indication to user
            iec_obj iobj;
            iobj.address = papdu->nsq60.ioa16 + ((unsigned)papdu->nsq60.ioa8 << 16);
            iobj.cause = papdu->asduh.cause;
            iobj.pn = papdu->asduh.pn;
            iobj.type = papdu->asduh.type;
            iobj.rcs = pobj->rcs;
            iobj.qu = pobj->qu;
            iobj.se = pobj->se;
            if (papdu->asduh.cause == ACTCONFIRM)
                commandActConfIndication(&iobj);
            else if (papdu->asduh.cause == ACTTERM)
                commandActTermIndication(&iobj);
        }
        break;

        case M_EI_NA_1: // 70
            mLog.pushMsg("--> END OF INITIALIZATION");
            // mLog.pushMsg("--> ��ʼ������");
            break;
        case INTERROGATION: // GI
            if (papdu->asduh.cause == ACTCONFIRM)
            {
                GIObjectCnt = 0;
                tout_gi = 0;
                mLog.pushMsg("    INTERROGATION ACT CON --------------------------------------------");
                // mLog.pushMsg("    �������� ��Ӧȷ�� --------------------------------------------");
                interrogationActConfIndication();
            }
            else if (papdu->asduh.cause == ACTTERM)
            {
                mLog.pushMsg("    INTERROGATION ACT TERM -------------------------------------------");
                // mLog.pushMsg("    �������� ��Ӧ���� -------------------------------------------");
                oss.str("");
                oss << "    Total objects in GI: "
                    // oss << "    �����ϴ�����: "
                    << GIObjectCnt;
                mLog.pushMsg((char *)oss.str().c_str());

                interrogationActTermIndication();
            }
            else
                mLog.pushMsg("    INTERROGATION");
            // mLog.pushMsg("    ��������");
            break;

        case INTEGRATEDTOTALS: // 101:������
            if (papdu->asduh.cause == ACTCONFIRM)
            {
                mLog.pushMsg("    INTEGRATEDTOTALS ACT CON ------------------------------------------------------------------------");
                // mLog.pushMsg("    ң������ ��Ӧȷ�� ---------------------------------------");
                ITObjectCnt = 0;
                integraltotalActConfIndication();
            }
            else if (papdu->asduh.cause == ACTTERM)
            {
                mLog.pushMsg("    INTEGRATEDTOTALS ACT TERM ----------------------------------------");
                // mLog.pushMsg("    ң������ ��Ӧ���� ----------------------------------------");
                bITreceived = true;
                oss.str("");
                oss << "    ң�������ϴ�����: "
                    << ITObjectCnt;
                mLog.pushMsg((char *)oss.str().c_str());
                integraltotalActTermIndication();
            }
            break;

        case C_TS_TA_1: // 107
            if (papdu->asduh.cause == ACTIVATION)
            {
                mLog.pushMsg("    TEST COMMAND COM TAG");
                // iec_type107 * ptype107;
                // ptype107=(iec_type107 *)papdu->dados;
                confTestCommand();
            }
            break;
        default:
            mLog.pushMsg("!!! TYPE NOT IMPLEMENTED");
            // mLog.pushMsg("!!! ��⵽δ��������");
            break;
        }

        if (accountandrespond)
        {

            tout_testfr = t3_testfr;

            if (msg_supervisory)
            {
                // will wait t2 seconds or n messages to send supervisory window control
                if (tout_supervisory < 0)
                    tout_supervisory = t2_supervisory;

                if (tout_supervisory > 0)
                    tout_supervisory--;

                if (tout_supervisory == 0)
                {
                    tout_supervisory = -1;
                    sendSupervisory();
                }
            }
            else
                sendSupervisory();
        }
    }
}

void iec104_class::sendSupervisory()
{
    stringstream oss;
    iec_apdu apdu;

    apdu.start = START;
    apdu.length = 4;
    apdu.NS = SUPERVISORY;
    apdu.NR = VR;
    sendTCP((char *)&apdu, 6);

    oss.str("");
    oss.setf(ios::hex, ios::basefield);
    oss << "<-- SUPERVISORY " << VR;
    mLog.pushMsg((char *)(oss.str().c_str()));
}

bool iec104_class::sendCommand(iec_obj *obj)
{
    iec_apdu apducmd;
    time_t tm1 = time(NULL);
    tm *agora = localtime(&tm1);
    stringstream oss;

    obj->cause = ACTIVATION;
    obj->ca = slaveAddress;

    switch (obj->type)
    {
    case C_SC_NA_1:
        apducmd.start = START;
        apducmd.length = sizeof(apducmd.NS) + sizeof(apducmd.NR) + sizeof(apducmd.asduh) + sizeof(apducmd.nsq45);
        apducmd.NS = VS;
        apducmd.NR = VR;
        apducmd.asduh.type = obj->type;
        apducmd.asduh.num = 1;
        apducmd.asduh.sq = 0;
        apducmd.asduh.cause = obj->cause;
        apducmd.asduh.t = 0;
        apducmd.asduh.pn = 0;
        apducmd.asduh.oa = masterAddress;
        apducmd.asduh.ca = obj->ca;
        apducmd.nsq45.ioa16 = obj->address & 0x0000FFFF;
        apducmd.nsq45.ioa8 = obj->address >> 16;
        apducmd.nsq45.obj.scs = obj->scs;
        apducmd.nsq45.obj.res = 0;
        apducmd.nsq45.obj.qu = obj->qu;
        apducmd.nsq45.obj.se = obj->se;
        sendTCP((char *)&apducmd, apducmd.length + sizeof(apducmd.start) + sizeof(apducmd.length));
        VS += 2;

        oss.str("");
        oss << "<-- SINGLE COMMAND ADDRESS "
            << (unsigned)obj->address
            << " SCS "
            << (unsigned)obj->scs
            << " QU "
            << (int)obj->qu
            << " SE "
            << (unsigned)obj->se;
        mLog.pushMsg((char *)oss.str().c_str());

        break;
    case C_DC_NA_1:
        apducmd.start = START;
        apducmd.length = sizeof(apducmd.NS) + sizeof(apducmd.NR) + sizeof(apducmd.asduh) + sizeof(apducmd.nsq46);
        apducmd.NS = VS;
        apducmd.NR = VR;
        apducmd.asduh.type = obj->type;
        apducmd.asduh.num = 1;
        apducmd.asduh.sq = 0;
        apducmd.asduh.cause = obj->cause;
        apducmd.asduh.t = 0;
        apducmd.asduh.pn = 0;
        apducmd.asduh.oa = masterAddress;
        apducmd.asduh.ca = obj->ca;
        apducmd.nsq46.ioa16 = obj->address & 0x0000FFFF;
        apducmd.nsq46.ioa8 = obj->address >> 16;
        apducmd.nsq46.obj.dcs = obj->dcs;
        apducmd.nsq46.obj.qu = obj->qu;
        apducmd.nsq46.obj.se = obj->se;
        sendTCP((char *)&apducmd, apducmd.length + sizeof(apducmd.start) + sizeof(apducmd.length));
        VS += 2;

        oss.str("");
        oss << "<-- DOUBLE COMMAND ADDRESS "
            << (unsigned)obj->address
            << " DCS "
            << (unsigned)obj->dcs
            << " QU "
            << (int)obj->qu
            << " SE "
            << (unsigned)obj->se;
        mLog.pushMsg((char *)oss.str().c_str());
        break;
    case C_RC_NA_1:
        apducmd.start = START;
        apducmd.length = sizeof(apducmd.NS) + sizeof(apducmd.NR) + sizeof(apducmd.asduh) + sizeof(apducmd.nsq47);
        apducmd.NS = VS;
        apducmd.NR = VR;
        apducmd.asduh.type = obj->type;
        apducmd.asduh.num = 1;
        apducmd.asduh.sq = 0;
        apducmd.asduh.cause = obj->cause;
        apducmd.asduh.t = 0;
        apducmd.asduh.pn = 0;
        apducmd.asduh.oa = masterAddress;
        apducmd.asduh.ca = obj->ca;
        apducmd.nsq47.ioa16 = obj->address & 0x0000FFFF;
        apducmd.nsq47.ioa8 = obj->address >> 16;
        apducmd.nsq47.obj.rcs = obj->rcs;
        apducmd.nsq47.obj.qu = obj->qu;
        apducmd.nsq47.obj.se = obj->se;
        sendTCP((char *)&apducmd, apducmd.length + sizeof(apducmd.start) + sizeof(apducmd.length));
        VS += 2;
        oss.str("");
        oss << "<-- STEP REG. COMMAND ADDRESS "
            << (unsigned)obj->address
            << " RCS "
            << (unsigned)obj->rcs
            << " QU "
            << (int)obj->qu
            << " SE "
            << (unsigned)obj->se;
        mLog.pushMsg((char *)oss.str().c_str());
        break;
    case C_SC_TA_1:
        apducmd.start = START;
        apducmd.length = sizeof(apducmd.NS) + sizeof(apducmd.NR) + sizeof(apducmd.asduh) + sizeof(apducmd.nsq58);
        apducmd.NS = VS;
        apducmd.NR = VR;
        apducmd.asduh.type = obj->type;
        apducmd.asduh.num = 1;
        apducmd.asduh.sq = 0;
        apducmd.asduh.cause = obj->cause;
        apducmd.asduh.t = 0;
        apducmd.asduh.pn = 0;
        apducmd.asduh.oa = masterAddress;
        apducmd.asduh.ca = obj->ca;
        apducmd.nsq58.ioa16 = obj->address & 0x0000FFFF;
        apducmd.nsq58.ioa8 = obj->address >> 16;
        apducmd.nsq58.obj.scs = obj->scs;
        apducmd.nsq58.obj.res = 0;
        apducmd.nsq58.obj.qu = obj->qu;
        apducmd.nsq58.obj.se = obj->se;
        apducmd.nsq58.obj.time.year = agora->tm_year % 100;
        apducmd.nsq58.obj.time.month = agora->tm_mon;
        apducmd.nsq58.obj.time.mday = agora->tm_mday;
        apducmd.nsq58.obj.time.hour = agora->tm_hour;
        apducmd.nsq58.obj.time.min = agora->tm_min;
        apducmd.nsq58.obj.time.hour = agora->tm_hour;
        apducmd.nsq58.obj.time.msec = agora->tm_sec * 1000;
        apducmd.nsq58.obj.time.iv = 0;
        apducmd.nsq58.obj.time.su = 0;
        apducmd.nsq58.obj.time.wday = agora->tm_wday;
        apducmd.nsq58.obj.time.res1 = 0;
        apducmd.nsq58.obj.time.res2 = 0;
        apducmd.nsq58.obj.time.res3 = 0;
        apducmd.nsq58.obj.time.res4 = 0;
        sendTCP((char *)&apducmd, apducmd.length + sizeof(apducmd.start) + sizeof(apducmd.length));
        VS += 2;

        oss.str("");
        oss << "<-- SINGLE COMMAND W/TIME ADDRESS "
            << (unsigned)obj->address
            << " SCS "
            << (unsigned)obj->scs
            << " QU "
            << (int)obj->qu
            << " SE "
            << (unsigned)obj->se;
        mLog.pushMsg((char *)oss.str().c_str());

        break;
    case C_DC_TA_1:
        apducmd.start = START;
        apducmd.length = sizeof(apducmd.NS) + sizeof(apducmd.NR) + sizeof(apducmd.asduh) + sizeof(apducmd.nsq59);
        apducmd.NS = VS;
        apducmd.NR = VR;
        apducmd.asduh.type = obj->type;
        apducmd.asduh.num = 1;
        apducmd.asduh.sq = 0;
        apducmd.asduh.cause = obj->cause;
        apducmd.asduh.t = 0;
        apducmd.asduh.pn = 0;
        apducmd.asduh.oa = masterAddress;
        apducmd.asduh.ca = obj->ca;
        apducmd.nsq59.ioa16 = obj->address & 0x0000FFFF;
        apducmd.nsq59.ioa8 = obj->address >> 16;
        apducmd.nsq59.obj.dcs = obj->dcs;
        apducmd.nsq59.obj.qu = obj->qu;
        apducmd.nsq59.obj.se = obj->se;
        apducmd.nsq59.obj.time.year = agora->tm_year % 100;
        apducmd.nsq59.obj.time.month = agora->tm_mon;
        apducmd.nsq59.obj.time.mday = agora->tm_mday;
        apducmd.nsq59.obj.time.hour = agora->tm_hour;
        apducmd.nsq59.obj.time.min = agora->tm_min;
        apducmd.nsq59.obj.time.hour = agora->tm_hour;
        apducmd.nsq59.obj.time.msec = agora->tm_sec * 1000;
        apducmd.nsq59.obj.time.iv = 0;
        apducmd.nsq59.obj.time.su = 0;
        apducmd.nsq59.obj.time.wday = agora->tm_wday;
        apducmd.nsq59.obj.time.res1 = 0;
        apducmd.nsq59.obj.time.res2 = 0;
        apducmd.nsq59.obj.time.res3 = 0;
        apducmd.nsq59.obj.time.res4 = 0;
        sendTCP((char *)&apducmd, apducmd.length + sizeof(apducmd.start) + sizeof(apducmd.length));
        VS += 2;

        oss.str("");
        oss << "<-- DOUBLE COMMAND W/TIME ADDRESS "
            << (unsigned)obj->address
            << " DCS "
            << (unsigned)obj->dcs
            << " QU "
            << (int)obj->qu
            << " SE "
            << (unsigned)obj->se;
        mLog.pushMsg((char *)oss.str().c_str());
        break;
    case C_RC_TA_1:
        apducmd.start = START;
        apducmd.length = sizeof(apducmd.NS) + sizeof(apducmd.NR) + sizeof(apducmd.asduh) + sizeof(apducmd.nsq60);
        apducmd.NS = VS;
        apducmd.NR = VR;
        apducmd.asduh.type = obj->type;
        apducmd.asduh.num = 1;
        apducmd.asduh.sq = 0;
        apducmd.asduh.cause = obj->cause;
        apducmd.asduh.t = 0;
        apducmd.asduh.pn = 0;
        apducmd.asduh.oa = masterAddress;
        apducmd.asduh.ca = obj->ca;
        apducmd.nsq60.ioa16 = obj->address & 0x0000FFFF;
        apducmd.nsq60.ioa8 = obj->address >> 16;
        apducmd.nsq60.obj.rcs = obj->rcs;
        apducmd.nsq60.obj.qu = obj->qu;
        apducmd.nsq60.obj.se = obj->se;
        apducmd.nsq60.obj.time.year = agora->tm_year % 100;
        apducmd.nsq60.obj.time.month = agora->tm_mon;
        apducmd.nsq60.obj.time.mday = agora->tm_mday;
        apducmd.nsq60.obj.time.hour = agora->tm_hour;
        apducmd.nsq60.obj.time.min = agora->tm_min;
        apducmd.nsq60.obj.time.hour = agora->tm_hour;
        apducmd.nsq60.obj.time.msec = agora->tm_sec * 1000;
        apducmd.nsq60.obj.time.iv = 0;
        apducmd.nsq60.obj.time.su = 0;
        apducmd.nsq60.obj.time.wday = agora->tm_wday;
        apducmd.nsq60.obj.time.res1 = 0;
        apducmd.nsq60.obj.time.res2 = 0;
        apducmd.nsq60.obj.time.res3 = 0;
        apducmd.nsq60.obj.time.res4 = 0;
        sendTCP((char *)&apducmd, apducmd.length + sizeof(apducmd.start) + sizeof(apducmd.length));
        VS += 2;
        oss.str("");
        oss << "<-- STEP REG. COMMAND W/TIME ADDRESS "
            << (unsigned)obj->address
            << " RCS "
            << (unsigned)obj->rcs
            << " QU "
            << (int)obj->qu
            << " SE "
            << (unsigned)obj->se;
        mLog.pushMsg((char *)oss.str().c_str());
        break;
    default:
        return false;
    }

    return true;
}

void iec104_class::setGICountDown(const int timeout)
{
    tout_gi = timeout;
    return;
}

void iec104_class::reverse(char *first, int size)
{
    if (first == NULL || size <= 0)
    {
        return;
    }
    int index = size;
    char *pstr = first;
    char *plast = &first[index - 1];
    for (int i = 0; i < (size / 2); i++)
    {
        swap(*pstr, *plast);
        pstr++;
        plast--;
    }
}