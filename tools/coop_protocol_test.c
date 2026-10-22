#include <stdio.h>
#include <string.h>

#include "coop_protocol.h"

static int sFails;

static void Expect(int cond, const char *name)
{
    if (!cond)
    {
        printf("FAIL %s\n", name);
        sFails++;
    }
    else
    {
        printf("ok %s\n", name);
    }
}

static void Fill(struct CoopFrame *frame)
{
    memset(frame, 0, sizeof(*frame));
    frame->buildXor = COOP_BUILD_XOR;
    frame->mapGroup = 1;
    frame->mapNum = 2;
    frame->x = 10;
    frame->y = 12;
    frame->facing = 2;
    frame->acro = 1;
    frame->capOn = 1;
    frame->request = COOP_REQ_CASE;
    frame->gender = 1;
    frame->bike = 1;
    frame->slot = 3;
    frame->badges = 0x15;
    frame->rivalStage = 2;
    frame->seq = 9;
    frame->legend[0] = COOP_OWNER_LOCAL;
    frame->legend[1] = COOP_OWNER_PARTNER;
    frame->legend[2] = COOP_OWNER_NONE;
}

int main(void)
{
    struct CoopFrame frame;
    struct CoopFrame out;
    u16 words[8];
    struct CoopProgress local;
    struct CoopProgress remote;
    struct CoopProgress merged;

    Fill(&frame);
    CoopProtocol_Pack(words, &frame, 0);
    Expect(CoopProtocol_Unpack(words, &out, COOP_BUILD_XOR) == COOP_UNPACK_OK, "roundtrip");
    Expect(out.x == 10 && out.y == 12 && out.acro == 1 && out.capOn == 1, "position and cap");
    Expect(out.request == COOP_REQ_CASE && out.slot == 3 && out.badges == 0x15, "request and badges");
    Expect(out.legend[0] == COOP_OWNER_LOCAL && out.legend[1] == COOP_OWNER_PARTNER, "legend owners");
    Expect(out.rivalStage == 2 && out.gender == 1 && out.bike == 1 && out.surf == 0, "rival and movement");

    words[7] ^= 1;
    Expect(CoopProtocol_Unpack(words, &out, COOP_BUILD_XOR) == COOP_UNPACK_DROP, "bad checksum");

    CoopProtocol_Pack(words, &frame, 0);
    words[2] = (u16)((70 << 8) | 1);
    words[7] = CoopProtocol_Checksum(words);
    Expect(CoopProtocol_Unpack(words, &out, COOP_BUILD_XOR) == COOP_UNPACK_DROP, "map group 70");

    CoopProtocol_Pack(words, &frame, 0);
    Expect(CoopProtocol_Unpack(words, &out, (u16)(COOP_BUILD_XOR ^ 1)) == COOP_UNPACK_BUILD, "build mismatch");

    CoopProtocol_Pack(words, &frame, 1);
    Expect(words[0] == COOP_HELD_KEYS_CMD, "held-keys command");
    Expect(CoopProtocol_Unpack(words, &out, COOP_BUILD_XOR) == COOP_UNPACK_OK, "held-keys accept");
    Expect(out.mapNum == 2 && out.slot == 3, "held-keys payload");

    memset(&local, 0, sizeof(local));
    memset(&remote, 0, sizeof(remote));
    local.badges = 0x01;
    local.rivalStage = 1;
    local.capOn = 1;
    local.legend[0] = COOP_OWNER_LOCAL;
    remote.badges = 0x02;
    remote.rivalStage = 3;
    remote.capOn = 0;
    Expect(CoopProtocol_Merge(&local, &remote, &merged) == 1, "cap mismatch");

    remote.capOn = 1;
    remote.legend[0] = COOP_OWNER_LOCAL;
    Expect(CoopProtocol_Merge(&local, &remote, &merged) == 2, "legend conflict");

    remote.legend[0] = COOP_OWNER_PARTNER;
    remote.legend[1] = COOP_OWNER_LOCAL;
    Expect(CoopProtocol_Merge(&local, &remote, &merged) == 0, "merge");
    Expect(merged.badges == 0x03 && merged.rivalStage == 3, "badges or, rival max");
    Expect(merged.legend[0] == COOP_OWNER_LOCAL, "agreed owner stays local");
    Expect(merged.legend[1] == COOP_OWNER_PARTNER, "remote local becomes partner");

    if (sFails != 0)
    {
        printf("%d failed\n", sFails);
        return 1;
    }
    printf("all passed\n");
    return 0;
}
