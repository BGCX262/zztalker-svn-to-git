#ifndef SHA1_H
#define SHA1_H

#include <qglobal.h>
#include <qbytearray.h>
#include <qstring.h>

QT_BEGIN_NAMESPACE

struct Sha1State
{
    quint32 h0;
    quint32 h1;
    quint32 h2;
    quint32 h3;
    quint32 h4;

    quint64 messageSize;
    unsigned char buffer[64];
};

typedef union
{
    quint8  bytes[64];
    quint32 words[16];
} Sha1Chunk;

void sha1InitState(Sha1State *state);
void sha1Update(Sha1State *state, const unsigned char *data, qint64 len);
void sha1FinalizeState(Sha1State *state);
void sha1ToHash(Sha1State *state, unsigned char* buffer);

QT_END_NAMESPACE

#endif // SHA1_H
