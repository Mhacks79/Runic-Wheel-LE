#pragma once
// Safe editor model for the original wheel catalog. Transactions are all-or-nothing.
#include "CatalogSave.h"
namespace rw04 {
struct Editor {
    static constexpr unsigned HistorySize=8;
    State undo[HistorySize]{};
    State redo[HistorySize]{};
    unsigned undoCount=0,redoCount=0;
    bool selecting=false;
    int sourceWheel=-1,sourceSector=-1,sourceEntry=-1;
};
inline void clear_selection(Editor& e){e.selecting=false;e.sourceWheel=e.sourceSector=e.sourceEntry=-1;}
inline void push(State (&records)[Editor::HistorySize],unsigned& count,const State& value){
    if(count==Editor::HistorySize){for(unsigned i=1;i<Editor::HistorySize;++i)records[i-1]=records[i];--count;}
    records[count++]=value;
}
inline bool undo(Editor& e,State& state){
    if(!e.undoCount)return false;
    push(e.redo,e.redoCount,state);
    state=e.undo[--e.undoCount];
    clear_selection(e);return true;
}
inline bool redo(Editor& e,State& state){
    if(!e.redoCount)return false;
    push(e.undo,e.undoCount,state);
    state=e.redo[--e.redoCount];
    clear_selection(e);return true;
}
inline bool valid(int wheel,int sector){return wheel>=0&&wheel<rw03::Wheels&&sector>=0&&sector<rw03::Slots;}
inline bool insert(Editor& e,State& state,int wheel,int sector,unsigned form){
    if(!valid(wheel,sector)||!form)return false;
    State trial=state;
    if(!rw03::add(trial.catalog.slots[wheel][sector],form))return false;
    push(e.undo,e.undoCount,state);e.redoCount=0;state=trial;return true;
}
inline bool erase(Editor& e,State& state,int wheel,int sector,int index){
    if(!valid(wheel,sector))return false;
    State trial=state;
    if(!rw03::remove(trial.catalog.slots[wheel][sector],index))return false;
    unsigned n=trial.catalog.slots[wheel][sector].count;
    trial.cursor[wheel][sector]=n?(unsigned char)(index<(int)n?index:n-1):0;
    push(e.undo,e.undoCount,state);e.redoCount=0;state=trial;clear_selection(e);return true;
}
inline bool move(Editor& e,State& state,int fromW,int fromS,int fromIndex,int toW,int toS){
    if(!valid(fromW,fromS)||!valid(toW,toS)|| (fromW==toW&&fromS==toS))return false;
    State trial=state;
    rw03::Slot& src=trial.catalog.slots[fromW][fromS];
    if(fromIndex<0||fromIndex>=src.count)return false;
    unsigned form=src.form[fromIndex];
    if(!rw03::add(trial.catalog.slots[toW][toS],form))return false;
    if(!rw03::remove(src,fromIndex))return false;
    unsigned n=src.count;
    trial.cursor[fromW][fromS]=n?(unsigned char)(fromIndex<(int)n?fromIndex:n-1):0;
    trial.cursor[toW][toS]=(unsigned char)(trial.catalog.slots[toW][toS].count-1);
    push(e.undo,e.undoCount,state);e.redoCount=0;state=trial;clear_selection(e);return true;
}
inline bool copy(Editor& e,State& state,int fromW,int fromS,int fromIndex,int toW,int toS){
    if(!valid(fromW,fromS)||!valid(toW,toS))return false;
    const rw03::Slot& source=state.catalog.slots[fromW][fromS];
    if(fromIndex<0||fromIndex>=source.count)return false;
    return insert(e,state,toW,toS,source.form[fromIndex]);
}
}
