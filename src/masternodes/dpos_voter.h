// Copyright (c) 2019 The Crypticcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef MASTERNODES_DPOS_VOTER_H
#define MASTERNODES_DPOS_VOTER_H

#include "dpos_p2p_messages.h"
#include "dpos_types.h"
#include "masternodes.h"
#include "../primitives/block.h"

namespace dpos
{

struct CDposVote
{
    CMasternode::ID voter;
    Round nRound = 0;
    BlockHash tip;
    CVoteChoice choice;
};

using CTxVote = CDposVote;
using CRoundVote = CDposVote;

struct CTxVotingDistribution
{
    size_t pro;
    size_t contra;
    size_t abstinendi;
    size_t totus() const;
};

struct CRoundVotingDistribution
{
    std::map<BlockHash, size_t> pro;
    size_t abstinendi;
    size_t totus() const;
};

/**
 * When voter finds that a vice-block is approved, it returns this object (as a part of CDposVoterOutput)
 */
struct CBlockToSubmit {
    CBlock block;
    Round nRound;
    std::vector<CMasternode::ID> vApprovedBy;
};

/**
 * Result of applying a new message to the voter agent.
 * Voter agent returns a new messages, which should be broadcasted to other agents.
 */
struct CDposVoterOutput
{
    std::vector<CDposVote> vTxVotes;
    std::vector<CDposVote> vRoundVotes;
    boost::optional<CBlockToSubmit> blockToSubmit;
    std::vector<std::string> vErrors;

    CDposVoterOutput& operator+=(const CDposVoterOutput& r);
    CDposVoterOutput operator+(const CDposVoterOutput& r);
};

class CDposVoter
{
public:
    using ValidateTxsF = std::function<bool(const std::map<TxIdSorted, CTransaction>&)>;
    using ValidateBlockF = std::function<bool(const CBlock&, const std::map<TxIdSorted, CTransaction>&, bool)>;
    using Output = CDposVoterOutput;

    /**
    * State of the voting at a specific block hash
    */
    struct VotingState
    {
        std::map<Round, std::map<TxId, std::map<CMasternode::ID, CDposVote> > > txVotes;
        std::map<Round, std::map<CMasternode::ID, CDposVote> > roundVotes;

        std::map<TxId, CTransaction> txs;
        std::map<BlockHash, CBlock> viceBlocks;
    };
    /**
    * Used to access blockchain
    */
    struct Callbacks
    {
        ValidateTxsF validateTxs;
        ValidateBlockF validateBlock;
    };

    mutable std::map<BlockHash, VotingState> v;
    size_t minQuorum;
    size_t numOfVoters;

    CDposVoter() = default;

    /**
    *
    * @param tip - current best block
    * @param world - blockchain callbacks
    * @param amIvoter is true if voting is enabled and I'm an active operator, member of the team
    * @param me is ID of current masternode
    */
    void setVoting(BlockHash tip,
                   Callbacks world,
                   bool amIvoter,
                   CMasternode::ID me);

    Output applyViceBlock(const CBlock& viceBlock);
    Output applyTx(const CTransaction& tx);
    Output applyTxVote(const CTxVote& vote);
    Output applyRoundVote(const CRoundVote& vote);

    /**
     * Force to vote PASS during this round, if round wasn't voted before.
     * Called when round didnt come to a consensus/stalemate for a long time.
     */
    Output doRoundVoting();
    Output doTxsVoting();
    Output onRoundTooLong();

    Round getCurrentRound() const;
    std::map<TxIdSorted, CTransaction> listCommittedTxs() const;

private:
    Output misbehavingErr(const std::string& msg) const;
    Output voteForTx(const CTransaction& tx);

    /**
     * @return transaction which had YES/NO vote from me, from any round. And transaction which had PASS vote from me in this round.
     */
    bool wasVotedByMe_tx(TxId txid, Round nRound) const;
    bool wasVotedByMe_round(Round nRound) const;

    /**
     * @return transaction which had YES vote from me, from any round
     */
    std::map<TxIdSorted, CTransaction> listApprovedByMe_txs() const;
    CTxVotingDistribution calcTxVotingStats(TxId txid, Round nRound) const;
    CRoundVotingDistribution calcRoundVotingStats(Round nRound) const;

    bool atLeastOneViceBlockIsValid(Round nRound) const;
    bool txHasAnyVote(TxId txid) const;
    bool wasTxLost(TxId txid) const;
    bool checkRoundStalemate(const CRoundVotingDistribution& stats) const;
    /**
     * Check that tx cannot be committed, due to already known votes
     */
    bool checkTxNotCommittable(const CTxVotingDistribution& stats) const;

    bool hasAnyUnfinishedTxs(Round nRound) const;

private:
    CMasternode::ID me;
    BlockHash tip;
    Callbacks world;
    bool amIvoter = false;
};

}

#endif //MASTERNODES_DPOS_VOTER_H