
#ifndef ANDROID_ATCOMMAND_H
#define ANDROID_ATCOMMAND_H

#if __cplusplus
extern "C" {
#endif

static int AT_NUM = 68;
static char* AT_CMD[][2] = {
    {"ATE0Q0V1","OK\n"},
    {"AT+CTEC?","+CTEC: 0,f\nOK\n"},
    {"AT+CTEC=?","+CTEC: 0,1,2,3\nOK\n"},
    {"ATE0Q0V1","OK\n"},
    {"ATS0=0","OK\n"},
    {"AT+CMEE=1","OK\n"},
    {"AT+CREG=2","OK\n"},
    {"AT+CGREG=1","OK\n"},
    {"AT+CCWA=1","OK\n"},
    {"AT+CMOD=0","OK\n"},
    {"AT+CMUT=0","OK\n"},
    {"AT+CSSN=0,1","OK\n"},
    {"AT+COLP=0","OK\n"},
    {"AT+CSCS=\"HEX\"","OK\n"},
    {"AT+CUSD=1","OK\n"},
    {"AT+CGEREP=1,0","OK\n"},
    {"AT+CMGF=0","OK\n"},
    {"AT+CFUN?","+CFUN: 0\nOK\n"},
    {"AT+CGSN","460000161369296\nOK\n"},
    {"AT+CFUN=1","OK\n"},
    {"AT+COPS=3,0;+COPS?;+COPS=3,1;+COPS?;+COPS=3,2;+COPS?","+COPS: 0,0,\"中国移动\"\n+COPS: 0,1,\"CMCC\"\n+COPS: 0,2,\"46000\"\nOK\n"},
    {"AT+CGREG?","+CGREG: 1,1,\"10cd\",\"51fa\",\"000e\"\nOK\n"},
    {"AT+CREG?","+CREG: 2,1,\"10cd\", \"51fa\"\nOK\n"},
    {"AT+COPS?","+COPS: 0,2,46000\nOK\n"},
    {"AT+CCHO=A00000015141434C00","1\nOK\n"},
    {"AT+CLCC","OK\n"},
    {"AT+CPIN?","+CPIN: READY\nOK\n"},
    {"AT+CGLA=1,10,80caff4000","+CGLA: 144,0,FF403eE23cE12eC11461ed377e85d386a8dfee6b864bd85b0bfaa5af81CA16616e64726f69642e636172726965726170692e637473E30aDB080000000000000000\nOK\n"},
    {"AT+CIMI","460000161369296\nOK\n"},
    {"AT+CRSM=176,12258,0,0,10","+CRSM: 144,0,98101430121181157002\nOK\n"},
    {"AT+CRSM=176,28433,0,0,1","+CRSM: 144,0,55\nOK\n"},
    {"AT+CRSM=176,28435,0,0,1","+CRSM: 144,0,55\nOK\n"},
    {"AT+CRSM=176,28436,0,0,20","+CRSM: 144,0,4368696e614d6f626c6965ffffffffffffffffff\nOK\n"},
    {"AT+CRSM=176,28438,0,0,2","+CRSM: 144,0,0233\nOK\n"},
    {"AT+CRSM=176,28472,0,0,15","+CRSM: 144,0,ff30ffff3c003c03000c0000f03f00\nOK\n"},
    {"AT+CRSM=176,28539,0,0,64","+CRSM: 144,0,ffffffffffffffffffffffff\nOK\n"},
    {"AT+CRSM=176,28589,0,0,4","+CRSM: 144,0,00000003\nOK\n"},
    {"AT+CRSM=178,28480,1,4,32","+CRSM: 144,0,ffffffffffffffffffffffffffffffffffff07813126909827f5ffffffffffff\nOK\n"},
    {"AT+CRSM=178,28613,1,4,24","+CRSM: 144,0,43058441aa890affffffffffffffffffffffffffffffffff\nOK\n"},
    {"AT+CRSM=178,28615,1,4,32","+CRSM: 144,0,566f6963656d61696cffffffffffffffffff07915155125740f9ffffffffffff\nOK\n"},
    {"AT+CRSM=178,28617,1,4,4","+CRSM: 144,0,01000000\nOK\n"},
    {"AT+CRSM=178,28618,1,4,5","+CRSM: 144,0,0000000000\nOK\n"},
    {"AT+CRSM=192,12258,0,0,15","+CRSM: 144,0,0000000a2fe204000fa0aa01020000\nOK\n"},
    {"AT+CRSM=192,28433,0,0,15","+CRSM: 144,0,000000016f11040011a0aa01020000\nOK\n"},
    {"AT+CRSM=192,28435,0,0,15","+CRSM: 144,0,000000016f13040011a0aa01020000\nOK\n"},
    {"AT+CRSM=192,28436,0,0,15","+CRSM: 144,0,000000146f1404001aa0aa01020000\nOK\n"},
    {"AT+CRSM=192,28438,0,0,15","+CRSM: 144,0,000000026f1604001aa0aa01020000\nOK\n"},
    {"AT+CRSM=192,28472,0,0,15","+CRSM: 144,0,0000000f6f3804001aa0aa01020000\nOK\n"},
    {"AT+CRSM=192,28480,0,0,15","+CRSM: 144,0,000000806f40040011a0aa01020120\nOK\n"},
    {"AT+CRSM=192,28486,0,0,15","+CRSM: 148,4\nOK\n"},
    {"AT+CRSM=192,28539,0,0,15","+CRSM: 144,0,000000406fc7040011a0aa01000000\nOK\n"},
    {"AT+CRSM=192,28589,0,0,15","+CRSM: 144,0,000000046fad04000aa0aa01020000\nOK\n"},
    {"AT+CRSM=192,28613,0,0,15","+CRSM: 144,0,000000f06fc504000aa0aa01020118\nOK\n"},
    {"AT+CRSM=192,28615,0,0,15","+CRSM: 144,0,000000406fc7040011a0aa01020120\nOK\n"},
    {"AT+CRSM=192,28617,0,0,15","+CRSM: 144,0,000000086fc9040011a0aa01020104\nOK\n"},
    {"AT+CRSM=192,28618,0,0,15","+CRSM: 144,0,0000000a6fca040011a0aa01020105\nOK\n"},
    {"AT+CRSM=192,28621,0,0,15","+CRSM: 148,4\nOK\n"},
    {"AT+CCHC=1","+CCHC\nOK\n"},
    {"AT+CGDCONT=1","OK\n"},
    {"AT+CGQREQ=1","OK\n"},
    {"AT+CGQMIN=1","OK\n"},
    {"AT+CGEREP=1,0","OK\n"},
    {"AT+CGACT=1,0","OK\n"},
    {"ATD*99***1#","OK\n"},
    {"AT+CGACT?","+CGACT: 1,1\nOK\n"},
    {"AT+CGDCONT?","+CGDCONT: 1,\"IP\",\"rmnet_data9\",\"%s/%s\",\"%s %s\",\"%s\"\nOK\n"},
    {"AT+CSQ","+CSQ:31,0,75,125,75,125,8,31,-1,-1,300,15\nOK\n"},
    {"AT+CMGS=30\n>\n004100038121f20004150605040710000041637469766174653a64743d3135^Z","+CMGS: 0\nOK\n"}
};
//{"AT+CRSM=176,28436,0,0,20","+CRSM: 144,0,416e64726f6964ffffffffffffffffffffffffff\nOK\n"},
//{"AT+CRSM=176,28436,0,0,20","+CRSM: 144,0,e4b8ade59bbde794b5e4bfa1ffffffffffffffff\nOK\n"},//中国电信

#if __cplusplus
};  // extern "C"
#endif
#endif //ANDROID_ATCOMMAND_H
