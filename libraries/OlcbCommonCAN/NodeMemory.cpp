
#include "NodeMemory.h"

#include "EventID.h"
#include "Event.h"
#include "NodeID.h"
#include "processor.h"

#include "lib_debug_print_common.h"


#define KEYSIZE 4
#define COUNTSIZE 2

// ToDo: NodeID* not kept in object member to save RAM space, may be false economy

NodeMemory::NodeMemory(int start) {
    startAddress = start;
    count = 0;
}

void NodeMemory::forceInitAll() {
    //LDEBUG("\nforceInitAll");
    EEPROM.update(0,0xFF);
    EEPROM.update(1,0xFF);
}

void NodeMemory::forceInitEvents() {
    //LDEBUG("\nforceInitEvents");
    EEPROM.update(2,0x33);
    EEPROM.update(3,0xCC);
}

extern void printEvents();
extern void initTables();
extern void userInitEventIDOffsets();
//void NodeMemory::setup(NodeID* nid, Event* cE, uint8_t nC, uint8_t* data, uint16_t extraBytes, uint16_t clearBytes) {
//void NodeMemory::setup(NodeID* nid, Event* cE, const uint16_t* eventidOffset, uint8_t nC, uint8_t* data, uint16_t extraBytes, uint16_t clearBytes) {
void NodeMemory::setup(Nodal_t* nodal, uint8_t* data, uint16_t extraBytes, uint16_t clearBytes) {
    //LDEBUG("\nIn NodeMemory::setup");
    NodeID* nid = nodal->nid;
    Event* cE  = nodal->events;
    const uint16_t* eventidOffset = nodal->eventidOffset;
    uint16_t nC = nodal->nevent;
    //userInitEventIDOffsets();
    //LDEBUG("\nnC(nevents)=");LDEBUG(nC);
    //for(int i=0;i<nC;i++) {
    //    LDEBUG("\n");LDEBUG(i);
    //    LDEBUG(":");LDEBUG2(eventidOffset[i],HEX);
    //}
    
    if (checkNidOK()) {
        //LDEBUG("\ncheckNidOK'd");
        
        // leave NodeID and count, reset rest
        
        // read NodeID from non-volative memory
        uint8_t* p;
        int addr = startAddress+KEYSIZE+COUNTSIZE; // skip check word and count
        p = (uint8_t*)nid;
        for (unsigned int i=0; i<sizeof(NodeID); i++) 
           *p++ = EEPROM.read(addr++);

        // load count
        uint8_t part1 = EEPROM.read(startAddress+KEYSIZE);
        uint8_t part2 = EEPROM.read(startAddress+KEYSIZE+1);
        count = (part1<<8)+part2;

        // handle the rest
        reset(nid, cE, eventidOffset, nC, clearBytes);

    } else if (!checkAllOK()) {
        //LDEBUG("\n!checkAllOK");
        // fires a factory reset
        count = 0;
        // handle the rest
        reset(nid, cE, eventidOffset, nC, clearBytes);
    }
    
    // read NodeID from non-volative memory
    uint8_t* p;
    int addr = startAddress+KEYSIZE+COUNTSIZE; // skip check word and count
    p = (uint8_t*)nid;
    for (uint8_t i=0; i<sizeof(NodeID); i++) 
        *p++ = EEPROM.read(addr++);

    /*
    // read events
    p = (uint8_t*)cE;
    for (uint8_t k=0; k<nC; k++)
        for (unsigned int i=0; i<sizeof(Event); i++) 
            *p++ = EEPROM.read(addr++);
    */
    
    initTables();
    
    // read extra data & load into RAM
    p = data;
    addr = KEYSIZE+COUNTSIZE+sizeof(NodeID)+nC*(sizeof(Event));
    for (uint8_t k=0; k<extraBytes; k++)
        *p++ = EEPROM.read(addr++);
    //LDEBUG("\nOut NodeMemory::setup");
}

//void NodeMemory::reset(NodeID* nid, Event* cE, uint8_t nC, uint16_t clearBytes) {
void NodeMemory::reset(NodeID* nid, Event* cE, const uint16_t* eventidOffset, uint8_t nC, uint16_t clearBytes) {
    //LDEBUG("\nNodeMemory::reset1");
    // Do the in-memory update. Does not reset
    // the total count to zero, this is not an "initial config" for factory use.

    // clear EEPROM
    //for (uint16_t i = 4; i<clearBytes; i++) EEPROM.update(i, 0);
    //store(nid, cE, nC, eOff);
    store(nid, cE, eventidOffset, nC);
    //LDEBUG("\nnC="); LDEBUG(nC);
    //while(0==0){}
    for (int e=0; e<nC; e++)
      //setToNewEventID(nid, cE[e].offset);
      setToNewEventID(nid, eventidOffset[e]);
}

//void NodeMemory::store(NodeID* nid, Event* cE, uint8_t nC, uint16_t* eOff) {
void NodeMemory::store(NodeID* nid, Event* cE, const uint16_t* eventidOffset, uint8_t nC) {
    
    int addr = startAddress;
    // write tag
    EEPROM.update(addr++, 0xEE);
    EEPROM.update(addr++, 0x55);
    EEPROM.update(addr++, 0x5E);
    EEPROM.update(addr++, 0xE5);

    // write count
    EEPROM.update(addr++, (count>>8)&0xFF);
    EEPROM.update(addr++, (count)&0xFF);
    
    // write NodeID
    uint8_t* p;
    p = (uint8_t*)nid;
    for (uint8_t i=0; i<sizeof(NodeID); i++) 
        EEPROM.update(addr++, *p++);

    // write events
    // don't need to rewrite the as they do not change --dph
    /*
    p = (uint8_t*)cE;
    for (int k=0; k<nC; k++) {
        for (uint8_t i=0; i<sizeof(EventID); i++) 
            EEPROM.update(addr++, *p++);
        for (uint8_t i=sizeof(EventID); i<sizeof(Event); i++) {
            // skip over the flags
            EEPROM.update(addr++, 0);
            p++;
         }
    }
     */

    // clear some memory
    //for (int k = 0; k < 64; k++) {
    //    EEPROM.update(addr++, 0);
    //}
}

void NodeMemory::store(NodeID* nid, Event* cE, const uint16_t* eventidOffset, uint8_t nC, uint8_t* data, int extraBytes) {
    store(nid, cE, eventidOffset, nC);
    // write extra data
    uint8_t* p = data;
    int addr = KEYSIZE+COUNTSIZE+sizeof(NodeID)+nC*(sizeof(Event));
    for (int k=0; k<extraBytes; k++)
        EEPROM.update(addr++, *p++);
}
void NodeMemory::storeToEEPROM(uint8_t *m, int n) {
    for (int i=0; i<n; i++)
        EEPROM.update(i, m[i]);
}

void NodeMemory::setToNewEventID(NodeID* nid, uint16_t offset) {
    //LDEBUG("\nIn setToNewEventID1");
    //LDEBUG(" offset="); LDEBUG2(offset,HEX);
    // All write to eeprom, may need to do a restore to RAM --dph
    uint16_t p = offset;  // this event's offset
    //LDEBUG(" ");LDEBUG2(p,HEX);LDEBUG(":");
    for (uint8_t i=0; i<sizeof(*nid); i++) {
      EEPROM.update(p++, nid->val[i]);
        //LDEBUG2(nid->val[i],HEX);LDEBUG(",");
    }
    count++;
    EEPROM.update(p++, (count>>8)&0xFF);
        //LDEBUG((count>>8)&0xFF);LDEBUG(",");
    EEPROM.update(p++, count&0xFF);
        //LDEBUG(count&0xFF);
}

bool NodeMemory::checkAllOK() {
    //LDEBUG("\ncheckAllOK");
    if (EEPROM.read(startAddress  ) != 0xEE ) return false;
    if (EEPROM.read(startAddress+1) != 0x55 ) return false;
    if (EEPROM.read(startAddress+2) != 0x5E ) return false;
    if (EEPROM.read(startAddress+3) != 0xE5 ) return false;
    return true;
}
bool NodeMemory::checkNidOK() {
    //LDEBUG("\ncheckNIDOK");
    if (EEPROM.read(startAddress  ) != 0xEE ) return false;
    if (EEPROM.read(startAddress+1) != 0x55 ) return false;
    if (EEPROM.read(startAddress+2) != 0x33 ) return false;
    if (EEPROM.read(startAddress+3) != 0xCC ) return false;
    return true;
}
