#ifndef BUNDLE_VIEW_V7_H
#define BUNDLE_VIEW_V7_H 1

#include "codec/bpv7.h"
#include <cstdint>
#include <vector>
#include <list>
#include <map>
#include <boost/asio/buffer.hpp>

/*
Each bundle SHALL be a concatenated sequence of at least two blocks,
represented as a CBOR indefinite-length array.  The first block in
the sequence (the first item of the array) MUST be a primary bundle
block in CBOR representation as described below; the bundle MUST
have exactly one primary bundle block. The primary block MUST be
followed by one or more canonical bundle blocks (additional array
items) in CBOR representation as described in 4.3.2 below.  Every
block following the primary block SHALL be the CBOR representation
of a canonical block.  The last such block MUST be a payload block;
the bundle MUST have exactly one payload block.  The payload block
SHALL be followed by a CBOR "break" stop code, terminating the
array.

(Note that, while CBOR permits considerable flexibility in the
encoding of bundles, this flexibility must not be interpreted as
inviting increased complexity in protocol data unit structure.)

Associated with each block of a bundle is a block number.  The block
number uniquely identifies the block within the bundle, enabling
blocks (notably bundle security protocol blocks) to reference other
blocks in the same bundle without ambiguity.  The block number of
the primary block is implicitly zero; the block numbers of all other
blocks are explicitly stated in block headers as noted below. Block
numbering is unrelated to the order in which blocks are sequenced in
the bundle. The block number of the payload block is always 1.

An implementation of the Bundle Protocol MAY discard any sequence of
bytes that does not conform to the Bundle Protocol specification.

An implementation of the Bundle Protocol MAY accept a sequence of
bytes that does not conform to the Bundle Protocol specification
(e.g., one that represents data elements in fixed-length arrays
rather than indefinite-length arrays) and transform it into
conformant BP structure before processing it.  Procedures for
accomplishing such a transformation are beyond the scope of this
specification.
*/

class BundleViewV7 {

public:
    struct Bpv7PrimaryBlockView {
        Bpv7CbhePrimaryBlock header;
        boost::asio::const_buffer actualSerializedPrimaryBlockPtr;
        bool dirty;

        void SetManuallyModified();
    };
    struct Bpv7CanonicalBlockView {
        Bpv7CanonicalBlockView();
        std::unique_ptr<Bpv7CanonicalBlock> headerPtr;
        boost::asio::const_buffer actualSerializedBlockPtr;
        bool dirty;
        bool markedForDeletion;
        bool isEncrypted;

        void SetManuallyModified();
    };
    
    BundleViewV7();
    ~BundleViewV7();

    void AppendMoveCanonicalBlock(std::unique_ptr<Bpv7CanonicalBlock> & headerPtr);
    void PrependMoveCanonicalBlock(std::unique_ptr<Bpv7CanonicalBlock> & headerPtr);
    bool InsertMoveCanonicalBlockAfterBlockNumber(std::unique_ptr<Bpv7CanonicalBlock> & headerPtr, const uint64_t blockNumber);
    bool InsertMoveCanonicalBlockBeforeBlockNumber(std::unique_ptr<Bpv7CanonicalBlock> & headerPtr, const uint64_t blockNumber);
    bool GetSerializationSize(uint64_t & serializationSize) const;
    std::size_t GetCanonicalBlockCountByType(const BPV7_BLOCK_TYPE_CODE canonicalBlockTypeCode) const;
    std::size_t GetNumCanonicalBlocks() const;
    void GetCanonicalBlocksByType(const BPV7_BLOCK_TYPE_CODE canonicalBlockTypeCode, std::vector<Bpv7CanonicalBlockView*> & blocks);
    uint64_t GetNextFreeCanonicalBlockNumber() const;
    std::size_t DeleteAllCanonicalBlocksByType(const BPV7_BLOCK_TYPE_CODE canonicalBlockTypeCode);
    bool LoadBundle(uint8_t * bundleData, const std::size_t size, const bool skipCrcVerifyInCanonicalBlocks = false, const bool loadPrimaryBlockOnly = false);
    bool SwapInAndLoadBundle(std::vector<uint8_t> & bundleData, const bool skipCrcVerifyInCanonicalBlocks = false, const bool loadPrimaryBlockOnly = false);
    bool CopyAndLoadBundle(const uint8_t * bundleData, const std::size_t size, const bool skipCrcVerifyInCanonicalBlocks = false, const bool loadPrimaryBlockOnly = false);
    bool IsValid() const;
    bool Render(const std::size_t maxBundleSizeBytes);
    bool RenderInPlace(const std::size_t paddingLeft);
    void Reset(); //should be private
private:
    bool Load(const bool skipCrcVerifyInCanonicalBlocks, const bool loadPrimaryBlockOnly);
    bool Render(uint8_t * serialization, uint64_t & sizeSerialized, bool terminateBeforeLastBlock);
    
public:
    Bpv7PrimaryBlockView m_primaryBlockView;
    const uint8_t * m_applicationDataUnitStartPtr;
    std::list<Bpv7CanonicalBlockView> m_listCanonicalBlockView; //list will maintain block relative order
    std::map<uint64_t, Bpv7BlockConfidentialityBlock*> m_mapEncryptedBlockNumberToBcbPtr;

    boost::asio::const_buffer m_renderedBundle;
    std::vector<uint8_t> m_frontBuffer;
    std::vector<uint8_t> m_backBuffer;
};

#endif // BUNDLE_VIEW_V7_H
