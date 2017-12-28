#ifndef MAIN_ZEROCOIN_H
#define MAIN_ZEROCOIN_H

#include "amount.h"
#include "chain.h"
#include "coins.h"
#include "consensus/validation.h"
#include "libzerocoin/Zerocoin.h"
#include "zerocoin_params.h"
#include <unordered_set>
#include <functional>

// Zerocoin transaction info, added to the CBlock to ensure zerocoin mint/spend transactions got their info stored into
// index
class CZerocoinTxInfo {
public:
    // all the zerocoin transactions encountered so far
    set<uint256> zcTransactions;
    // <denomination, pubCoin> for all the mints
    vector<pair<int,CBigNum> > mints;
    // serial for every spend
    set<CBigNum> spentSerials;

    // information about transactions in the block is complete
    bool fInfoIsComplete;

    CZerocoinTxInfo(): fInfoIsComplete(false) {}
    // finalize everything
    void Complete();
};

bool CheckZerocoinFoundersInputs(const CTransaction &tx, CValidationState &state, int nHeight, bool fTestNet);
bool CheckZerocoinTransaction(const CTransaction &tx,
	CValidationState &state,
	uint256 hashTx,
	bool isVerifyDB,
	int nHeight,
    bool isCheckWallet,
    CZerocoinTxInfo *zerocoinTxInfo);

void DisconnectTipZC(CBlock &block, CBlockIndex *pindexDelete);
bool ConnectTipZC(CValidationState &state, const CChainParams &chainparams, CBlockIndex *pindexNew, const CBlock *pblock);

int ZerocoinGetNHeight(const CBlockHeader &block);

bool ZerocoinBuildStateFromIndex(CChain *chain);

/*
 * State of minted/spent coins as extracted from the index
 */
class CZerocoinState {
public:
    // First and last block where mint (and hence accumulator update) with given denomination and id was seen
    struct CoinGroupInfo {
        CoinGroupInfo() : firstBlock(NULL), lastBlock(NULL), nCoins(0) {}

        // first and last blocks having coins with given denomination and id minted
        CBlockIndex *firstBlock;
        CBlockIndex *lastBlock;
        // total number of minted coins with such parameters
        int nCoins;
    };

private:
    // Custom hash for big numbers
    struct CBigNumHash {
        std::size_t operator()(const CBigNum &bn) const noexcept;
    };

    // Collection of coin groups. Map from <denomination,id> to CoinGroupInfo structure
    map<pair<int, int>, CoinGroupInfo> coinGroups;
    // Set of all used coin serials. Allows multiple entries for the same coin serial for historical reasons
    unordered_multiset<CBigNum,CBigNumHash> usedCoinSerials;
    // Set of all minted pubCoin values
    unordered_multiset<CBigNum,CBigNumHash> mintedPubCoins;
    // Latest IDs of coins by denomination
    map<int, int> latestCoinIds;

public:
    CZerocoinState();

    // Add mint, automatically assigning id to it. Returns id and previous accumulator value (if any)
    int AddMint(CBlockIndex *index, int denomination, const CBigNum &pubCoin, CBigNum &previousAccValue);
    // Add serial to the list of used ones
    void AddSpend(const CBigNum &serial);

    // Add everything from the block to the state
    void AddBlock(CBlockIndex *index);
    // Disconnect block from the chain rolling back mints and spends
    void RemoveBlock(CBlockIndex *index);

    // Query coin group with given denomination and id
    bool GetCoinGroupInfo(int denomination, int id, CoinGroupInfo &result);

    // Query if the coin serial was previously used
    bool IsUsedCoinSerial(const CBigNum &coinSerial);
    // Query if there is a coin with given pubCoin value
    bool HasCoin(const CBigNum &pubCoin);

    // Given denomination and id returns latest accumulator value and corresponding block hash
    // Do not take into account coins with height more than maxHeight
    // Returns number of coins satisfying conditions
    int GetAccumulatorValueForSpend(int maxHeight, int denomination, int id, CBigNum &accumulator, uint256 &blockHash);

    // Reset to initial values
    void Reset();

    static CZerocoinState *GetZerocoinState();
};

#endif