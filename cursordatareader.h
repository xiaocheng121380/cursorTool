#ifndef CURSORDATAREADER_H
#define CURSORDATAREADER_H

#include "cursordata.h"
#include <QString>

class CursorDataReader {
public:
    // 读取Cursor会话数据的静态方法
    static CursorSessionData readSessionData();
};

#endif // CURSORDATAREADER_H 