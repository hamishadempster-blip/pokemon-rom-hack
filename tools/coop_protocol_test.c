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

    memset(&local, 0, sizeof(local));
    memset(&remote, 0, sizeof(remote));
    local.capOn = remote.capOn = 1;
    local.badges = 0x01;
    local.rivalStage = 0;
    remote.badges = 0x7F;
    remote.rivalStage = 3;
    remote.legend[1] = COOP_OWNER_LOCAL;
    Expect(CoopProtocol_Merge(&local, &remote, &merged) == 0, "older save reconnects");
    Expect(merged.badges == 0x7F, "older save gains every joint badge");
    Expect(merged.rivalStage == 3, "older save gains the higher rival stage");
    Expect(merged.legend[0] == COOP_OWNER_NONE && merged.legend[1] == COOP_OWNER_PARTNER && merged.legend[2] == COOP_OWNER_NONE,
           "older save learns the partner owns Kyogre");
    Expect(sizeof(struct CoopProgress) == 6, "progress carries only badges, rival, cap, and owners");
    Expect(CoopProtocol_Merge(&remote, &local, &merged) == 0 && merged.badges == 0x7F && merged.rivalStage == 3,
           "newer save keeps its badges and rival stage");

    local.legend[1] = COOP_OWNER_LOCAL;
    Expect(CoopProtocol_Merge(&local, &remote, &merged) == 2, "both saves claiming Kyogre drops the link");
    Expect(CoopProtocol_Merge(&remote, &local, &merged) == 2, "Kyogre conflict either way round");

    Expect(CoopProtocol_KeepOwnInvite(0, 1, 1, 1) == 1, "link player 0 keeps its Elite Four invite");
    Expect(CoopProtocol_KeepOwnInvite(1, 1, 1, 1) == 0, "link player 1 joins player 0's invite");
    Expect(CoopProtocol_KeepOwnInvite(0, 0, 1, 1) == 0, "no own invite means join");
    Expect(CoopProtocol_KeepOwnInvite(0, 1, 0, 1) == 0, "an optional wait yields to a mandatory fight");
    Expect(CoopProtocol_KeepOwnInvite(0, 1, 1, 0) == 0, "an optional invite is not held off by this rule");

    Expect(COOP_LINK_QUIET_LIMIT == 240, "quiet cable matches the gpSP Gen 3 peer timeout");
    Expect(CoopProtocol_RaiseVar(0, 1) == 1, "a guest story var moves forward");
    Expect(CoopProtocol_RaiseVar(8, 7) == 8, "a later story var stays");
    Expect(CoopProtocol_RaiseVar(6, 6) == 6, "an equal story var stays");
    Expect(CoopProtocol_TrainerCategory(1, 1, 0) == COOP_CAT_SOLO, "the frontier stays solo");
    Expect(CoopProtocol_TrainerCategory(0, 1, 1) == COOP_CAT_SOLO, "a couple stays solo");
    Expect(CoopProtocol_TrainerCategory(0, 1, 0) == COOP_CAT_MANDATORY, "a villain is mandatory");
    Expect(CoopProtocol_TrainerCategory(0, 0, 0) == COOP_CAT_OPTIONAL, "an ordinary trainer is optional");
    Expect(CoopProtocol_QueuesCoop(COOP_CAT_MANDATORY, 0, 1) == 1, "a villain ignores the beacon");
    Expect(CoopProtocol_QueuesCoop(COOP_CAT_OPTIONAL, 0, 1) == 0, "an optional trainer needs the beacon");
    Expect(CoopProtocol_QueuesCoop(COOP_CAT_OPTIONAL, 1, 0) == 0, "no battle across maps");
    Expect(CoopProtocol_QueuesCoop(COOP_CAT_SOLO, 1, 1) == 0, "a solo category never queues");
    Expect(CoopProtocol_DistinctPartner(30, 30) == 0, "one grunt id is not two grunts");
    Expect(CoopProtocol_DistinctPartner(30, 0) == 0, "a missing partner stays empty");
    Expect(CoopProtocol_DistinctPartner(30, 0xFFFF) == 0, "a follower sentinel is not a second trainer");

    {
        s16 x = 8, y = 5;
        CoopProtocol_VisiblePartnerTile(8, 5, &x, &y);
        Expect(x == 8 && y == 6, "a shared tile draws the partner one step south");
        x = 4;
        y = 9;
        CoopProtocol_VisiblePartnerTile(8, 5, &x, &y);
        Expect(x == 4 && y == 9, "a different tile stays where they are standing");
    }
    Expect(CoopProtocol_DistinctPartner(30, 31) == 31, "two different trainers both fight");

    Expect(CoopProtocol_SameMap(3, 10, 3, 10) == 1, "same map shows the sprite");
    Expect(CoopProtocol_SameMap(3, 10, 4, 10) == 0, "different map hides the sprite");
    Expect(CoopProtocol_SameMap(3, 10, 3, 11) == 0, "a door drops the sprite");

    frame.request = COOP_REQ_BEACON;
    frame.slot = 1;
    CoopProtocol_Pack(words, &frame, 0);
    Expect(CoopProtocol_Unpack(words, &out, COOP_BUILD_XOR) == COOP_UNPACK_OK, "beacon request roundtrip");
    Expect(out.request == COOP_REQ_BEACON && out.slot == 1, "beacon request payload");

    {
        struct CoopBeaconState a;
        struct CoopBeaconState b;
        u8 aReq, aSlot, bReq, bSlot;

        memset(&a, 0, sizeof(a));
        memset(&b, 0, sizeof(b));
        CoopProtocol_BeaconOffer(&a, 1);
        CoopProtocol_BeaconStep(&b, a.request, a.slot, 0);
        Expect(b.event == COOP_BEACON_EVT_PROMPT && b.on == 0, "partner is asked and beacon stays off");
        CoopProtocol_BeaconAnswer(&b, 0);
        aReq = a.request;
        aSlot = a.slot;
        bReq = b.request;
        bSlot = b.slot;
        CoopProtocol_BeaconStep(&a, bReq, bSlot, 0);
        CoopProtocol_BeaconStep(&b, aReq, aSlot, 0);
        Expect(a.on == 0 && a.event == COOP_BEACON_EVT_DECLINED, "a refusal leaves the beacon off");

        memset(&a, 0, sizeof(a));
        memset(&b, 0, sizeof(b));
        CoopProtocol_BeaconOffer(&a, 1);
        CoopProtocol_BeaconStep(&b, a.request, a.slot, 1);
        Expect(b.event == COOP_BEACON_EVT_NONE && b.on == 0, "a gym freezes the request");
        CoopProtocol_BeaconStep(&b, a.request, a.slot, 0);
        CoopProtocol_BeaconAnswer(&b, 1);
        Expect(b.on == 0, "a local yes does not turn the beacon on");
        aReq = a.request;
        aSlot = a.slot;
        bReq = b.request;
        bSlot = b.slot;
        CoopProtocol_BeaconStep(&a, bReq, bSlot, 0);
        CoopProtocol_BeaconStep(&b, aReq, aSlot, 0);
        Expect(a.on == 0 && b.on == 0 && a.phase == COOP_BEACON_CONFIRM, "the first yes only echoes");
        aReq = a.request;
        aSlot = a.slot;
        bReq = b.request;
        bSlot = b.slot;
        CoopProtocol_BeaconStep(&a, bReq, bSlot, 0);
        CoopProtocol_BeaconStep(&b, aReq, aSlot, 0);
        Expect(a.on == 1 && b.on == 1, "both yes packets turn the beacon on");
        Expect(a.event == COOP_BEACON_EVT_APPLIED && b.event == COOP_BEACON_EVT_APPLIED, "both sides apply together");

        memset(&a, 0, sizeof(a));
        memset(&b, 0, sizeof(b));
        a.on = 1;
        b.on = 1;
        CoopProtocol_BeaconOffer(&a, 0);
        CoopProtocol_BeaconOffer(&b, 0);
        aReq = a.request;
        aSlot = a.slot;
        bReq = b.request;
        bSlot = b.slot;
        CoopProtocol_BeaconStep(&a, bReq, bSlot, 0);
        CoopProtocol_BeaconStep(&b, aReq, aSlot, 0);
        Expect(a.on == 1 && b.on == 1, "an off request waits for the echo");
        aReq = a.request;
        aSlot = a.slot;
        bReq = b.request;
        bSlot = b.slot;
        CoopProtocol_BeaconStep(&a, bReq, bSlot, 1);
        CoopProtocol_BeaconStep(&b, aReq, aSlot, 1);
        Expect(a.on == 1 && b.on == 1, "the gym keeps the old beacon");
        aReq = a.request;
        aSlot = a.slot;
        bReq = b.request;
        bSlot = b.slot;
        CoopProtocol_BeaconStep(&a, bReq, bSlot, 0);
        CoopProtocol_BeaconStep(&b, aReq, aSlot, 0);
        Expect(a.on == 0 && b.on == 0, "the beacon clears after the gym");
    }

    {
        struct CoopMonState party[COOP_ROSTER_SLOTS];
        struct CoopMonState snap[COOP_ROSTER_SLOTS];
        struct CoopMonState live[COOP_ROSTER_SLOTS];
        int i, ok;

        memset(party, 0, sizeof(party));
        for (i = 0; i < COOP_ROSTER_SLOTS; i++)
        {
            party[i].species = (u16)(100 + i);
            party[i].hp = 0;
        }
        party[4].hp = 50;
        Expect(!CoopProtocol_LeadReady(party), "a healthy fifth slot does not make the lead ready");
        party[2].hp = 1;
        Expect(CoopProtocol_LeadReady(party), "one conscious mon in the first three is enough");
        party[2].isEgg = 1;
        Expect(!CoopProtocol_LeadReady(party), "an egg in the first three cannot battle");
        memset(party, 0, sizeof(party));
        party[0].species = 25;
        party[0].hp = 20;
        Expect(CoopProtocol_LeadReady(party), "a one-mon party can join");

        Expect(!CoopProtocol_RestoreFromSnapshot(0, 0) && !CoopProtocol_RestoreFromSnapshot(2, 0), "a finished battle keeps slots 1-3");
        Expect(CoopProtocol_RestoreFromSnapshot(3, 0) && CoopProtocol_RestoreFromSnapshot(5, 0), "a finished battle restores slots 4-6");
        Expect(CoopProtocol_RestoreFromSnapshot(0, 1) && CoopProtocol_RestoreFromSnapshot(5, 1), "an abort restores all six");
        Expect(!CoopProtocol_RestoreFromSnapshot(6, 1), "nothing past slot 6");

        for (i = 0; i < COOP_ROSTER_SLOTS; i++)
        {
            snap[i].species = (u16)(200 + i);
            snap[i].hp = (u16)(40 + i);
            snap[i].isEgg = 0;
        }
        memcpy(live, snap, sizeof(live));
        for (i = COOP_BATTLE_SLOTS; i < COOP_ROSTER_SLOTS; i++)
            memset(&live[i], 0, sizeof(live[i]));
        live[0].hp = 0;
        live[1].hp = 7;
        for (i = 0; i < COOP_ROSTER_SLOTS; i++)
        {
            if (CoopProtocol_RestoreFromSnapshot((u8)i, 0))
                live[i] = snap[i];
        }
        ok = live[0].hp == 0 && live[1].hp == 7 && live[2].hp == 42;
        for (i = COOP_BATTLE_SLOTS; i < COOP_ROSTER_SLOTS; i++)
            ok = ok && live[i].species == snap[i].species && live[i].hp == snap[i].hp;
        Expect(ok, "slots 1-3 keep the fight and slots 4-6 come back as they were");
    }

    {
        struct CoopCatchLedger ledger;
        const u8 host = 1, guest = 2;

        CoopProtocol_ClearCatches(&ledger);
        Expect(CoopProtocol_ClaimCatch(&ledger, 0, host, 1), "the first ball at wild A catches it");
        Expect(!CoopProtocol_ClaimCatch(&ledger, 0, guest, 1), "a second ball at A in the same turn catches nothing");
        Expect(ledger.owner[0] == host, "A keeps exactly one owner");
        Expect(CoopProtocol_CatchCount(&ledger, host) == 1 && CoopProtocol_CatchCount(&ledger, guest) == 0, "only the catcher holds A");
        Expect(ledger.owner[1] == COOP_THROWER_NONE, "leaving B alone leaves B unowned");

        CoopProtocol_ClearCatches(&ledger);
        Expect(CoopProtocol_ClaimCatch(&ledger, 0, host, 1) && CoopProtocol_ClaimCatch(&ledger, 1, guest, 1), "different targets are both caught");
        Expect(CoopProtocol_CatchCount(&ledger, host) == 1 && CoopProtocol_CatchCount(&ledger, guest) == 1, "each human owns the one they caught");

        CoopProtocol_ClearCatches(&ledger);
        Expect(!CoopProtocol_ClaimCatch(&ledger, 1, guest, 0), "a target that already left the field is not caught");
        Expect(!CoopProtocol_ClaimCatch(&ledger, 2, guest, 1), "there are only two wild slots");
        Expect(!CoopProtocol_ClaimCatch(&ledger, 0, COOP_THROWER_NONE, 1), "nobody cannot own a catch");

        CoopProtocol_ClaimCatch(&ledger, 0, guest, 1);
        CoopProtocol_ClearCatches(&ledger);
        Expect(ledger.owner[0] == COOP_THROWER_NONE && ledger.owner[1] == COOP_THROWER_NONE, "a dropped cable mid-shake leaves the catch with nobody");
    }

    {
        u8 run[2], act[2];

        run[0] = 1; run[1] = 1; act[0] = 1; act[1] = 1;
        Expect(CoopProtocol_FleeAgreed(run, act, 2), "both chose Run, so the fight ends");
        run[1] = 0;
        Expect(!CoopProtocol_FleeAgreed(run, act, 2), "one Run alone does not end the fight");
        act[1] = 0;
        Expect(CoopProtocol_FleeAgreed(run, act, 2), "a human with nothing standing does not block the run");
        act[0] = 0;
        Expect(!CoopProtocol_FleeAgreed(run, act, 2), "nobody standing is not a run");
    }

    {
        Expect(CoopProtocol_CatchRoom(6, 0, 1), "a full party sends the catch to the PC");
        Expect(!CoopProtocol_CatchRoom(6, 0, 0), "a full party and a full PC refuse the throw");
        Expect(CoopProtocol_CatchRoom(5, 0, 0), "an open party slot is room");
        Expect(!CoopProtocol_CatchRoom(5, 1, 0), "an earlier catch this fight holds the last slot");
        Expect(CoopProtocol_CatchRoom(6, 1, 2), "two PC spaces cover a second catch");
    }

    {
        u8 none[2] = {0, 0};
        u8 left[2] = {1, 0};
        u8 right[2] = {0, 1};
        u8 beat[2] = {1, 0};
        u8 both[2] = {1, 1};

        Expect(CoopProtocol_PairPlan(none, none, 1, 1) == COOP_PAIR_JOINT, "both ids clear on both saves and both said yes");
        Expect(CoopProtocol_PairPlan(none, none, 1, 0) == COOP_PAIR_SOLO, "a no from either player keeps it solo");
        Expect(CoopProtocol_PairPlan(none, none, 0, 1) == COOP_PAIR_SOLO, "a busy partner keeps it solo");
        Expect(CoopProtocol_PairPlan(left, none, 1, 1) == COOP_PAIR_SOLO, "this save beat the left id, so no joint fight");
        Expect(CoopProtocol_PairPlan(none, right, 1, 1) == COOP_PAIR_SOLO, "the other save beat the right id, so no joint fight");

        CoopProtocol_PairFlagsAfter(beat, both, 0);
        Expect(beat[0] == 1 && beat[1] == 0, "a loss or a cable pull sets no new flag");
        CoopProtocol_PairFlagsAfter(none, both, 1);
        Expect(none[0] == 1 && none[1] == 1, "a joint win marks both fought ids");
        CoopProtocol_PairFlagsAfter(right, left, 1);
        Expect(right[0] == 1 && right[1] == 1, "a win marks only the ids that were fought");
        Expect(COOP_PAIR_LEFT_ID != COOP_PAIR_RIGHT_ID, "the two ids are different");
    }

    if (sFails != 0)
    {
        printf("%d failed\n", sFails);
        return 1;
    }
    printf("all passed\n");
    return 0;
}
