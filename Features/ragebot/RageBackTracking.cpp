/*
            By Semmxz
*/

#include "RageBackTracking.h"
#include "Aim.h"
#include "..\Lagcompensation\Animation_system.h"
#include "Antiaim.h"

static std::deque<CIncomingSequence>sequences;
static int lastincomingsequencenumber;
int Real_m_nInSequencenumber;


void CBacktracking::UpdateIncomingSequences()
{
    if (m_clientstate())
    {
        if (m_clientstate()->pNetChannel)
        {
            if (m_clientstate()->pNetChannel->m_nInSequenceNr > lastincomingsequencenumber)
            {
                lastincomingsequencenumber = m_clientstate()->pNetChannel->m_nInSequenceNr;
                sequences.push_front(CIncomingSequence(m_clientstate()->pNetChannel->m_nInReliableState, m_clientstate()->pNetChannel->m_nOutReliableState,
                    m_clientstate()->pNetChannel->m_nInSequenceNr, m_globals()->m_realtime));
            }

            if (sequences.size() > 2048)
                sequences.pop_back();
        }
    }
}

void CBacktracking::ClearIncomingSequences()
{
    sequences.clear();
}

void CBacktracking::AddLatencyToNetchan(INetChannel* netchan, float Latency)
{
    for (auto& seq : sequences)
    {
        if (m_globals()->m_realtime - seq.curtime >= Latency
            || m_globals()->m_realtime - seq.curtime > 1)
        {
            netchan->m_nInReliableState = seq.inreliablestate;
            netchan->m_nInSequenceNr = seq.sequencenr;
            break;
        }
    }
}