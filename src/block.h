#ifndef CBLOCK_H
#define CBLOCK_H

#include "uint256.h"
#include "serialize.h"

class CTxDB;
class CKeyStore;
class CTransaction;
class COutPoint;
class CBlockIndex;
class CHeaderChainDB;

typedef struct block_header_s
{
    unsigned int version;
    uint256 prev_block;
    uint256 merkle_root;
    unsigned int timestamp;
    unsigned int bits;
    unsigned int nonce;

} block_header;

/** Nodes collect new transactions into a block, hash them into a hash tree,
 * and scan through nonce values to make the block's hash satisfy proof-of-work
 * requirements.  When they solve the proof-of-work, they broadcast the block
 * to everyone and the block is added to the block chain.  The first transaction
 * in the block is a special one that creates a new coin owned by the creator
 * of the block.
 *
 * Blocks are appended to blk0001.dat files on disk.  Their location on disk
 * is indexed by CBlockIndex objects in memory.
 */
class CBlock
{
public:
    // header
    static const int CURRENT_VERSION = 4;
    int nVersion;
    uint256 hashPrevBlock;
    uint256 hashMerkleRoot;
    unsigned int nTime;
    unsigned int nBits;
    unsigned int nNonce;

    // network and disk
    std::vector<CTransaction> vtx;

    // ppcoin: block signature - signed by one of the coin base txout[N]'s owner
    std::vector<unsigned char> vchBlockSig;

    // memory only
    mutable std::vector<uint256> vMerkleTree;

    // Denial-of-service detection:
    mutable int nDoS;
    bool DoS(int nDoSIn, bool fIn) const { nDoS += nDoSIn; return fIn; }

    CBlock()
    {
        SetNull();
    }

    IMPLEMENT_SERIALIZE
    (
        READWRITE(this->nVersion);
        nVersion = this->nVersion;
        READWRITE(hashPrevBlock);
        READWRITE(hashMerkleRoot);
        READWRITE(nTime);
        READWRITE(nBits);
        READWRITE(nNonce);

        // ConnectBlock depends on vtx following header to generate CDiskTxPos
        if (!(nType & (SER_GETHASH|SER_BLOCKHEADERONLY)))
        {
            READWRITE(vtx);
            READWRITE(vchBlockSig);
        }
        else if (fRead)
        {
            const_cast<CBlock*>(this)->vtx.clear();
            const_cast<CBlock*>(this)->vchBlockSig.clear();
        }
    )

    void SetNull();
    bool IsNull() const;
    uint256 GetHash() const;
    int64_t GetBlockTime() const;
    void UpdateTime(const CBlockIndex* pindexPrev);
    unsigned int GetStakeEntropyBit(unsigned int nHeight) const;
    bool IsProofOfStake() const;
    bool IsProofOfWork() const;
    std::pair<COutPoint, unsigned int> GetProofOfStake() const;
    int64_t GetMaxTransactionTime() const;
    uint256 BuildMerkleTree() const;
    std::vector<uint256> GetMerkleBranch(int nIndex) const;
    uint256 CheckMerkleBranch(uint256 hash, const std::vector<uint256>& vMerkleBranch, int nIndex);
    bool WriteToDisk(unsigned int& nFileRet, unsigned int& nBlockPosRet);
    bool ReadFromDisk(unsigned int nFile, unsigned int nBlockPos, bool fReadTransactions=true);
    void print() const;
    void printScrypt() const;
    bool DisconnectBlock(CTxDB& txdb, CHeaderChainDB& hcdb, CBlockIndex* pindex);
    bool ConnectBlock(CTxDB& txdb, CHeaderChainDB &hcdb, CBlockIndex* pindex, bool fJustCheck=false);
    bool ReadFromDisk(const CBlockIndex* pindex, bool fReadTransactions=true);
    bool SetBestChain(CTxDB& txdb, CHeaderChainDB& hcdb, CBlockIndex* pindexNew);
    bool AddToOrphanTracker(unsigned int nFile, unsigned int nBlockPos, const uint256& hashProofOfStake);
    bool AddToBlockIndex(unsigned int nFile, unsigned int nBlockPos, const uint256& hashProofOfStake);
    bool CheckBlock(bool fCheckPOW=true, bool fCheckMerkleRoot=true, bool fCheckSig=true) const;
    bool AcceptBlock(CBlock* pblock);
    bool GetCoinAge(uint64_t& nCoinAge) const; // ppcoin: calculate total coin age spent in block
    bool SignScryptBlock(const CKeyStore& keystore);
    bool CheckBlockSignature() const;

private:
    bool SetBestChainInner(CTxDB& txdb, CHeaderChainDB &hcdb, CBlockIndex *pindexNew);
};


extern CBlockIndex* FindBlockByHeight(int nHeight);

#endif // CBLOCK_H