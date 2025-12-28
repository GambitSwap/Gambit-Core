# Gambit: A Multi-VM Zero-Knowledge Blockchain

## Abstract

Gambit is a next-generation blockchain platform that combines the security and compatibility of Ethereum with the scalability benefits of zero-knowledge proofs and multi-virtual-machine architecture. Built in modern C++17, Gambit introduces a novel approach to blockchain design by supporting three distinct virtual machines—EVM, WASM, and native Core contracts—while integrating zero-knowledge validity proofs into its consensus mechanism. This whitepaper presents Gambit's technical architecture, consensus model, and unique innovations that position it as a foundation for scalable, interoperable decentralized applications.

## 1. Introduction

### 1.1 Background

Traditional blockchain platforms face fundamental scalability and interoperability challenges. Ethereum, while pioneering smart contracts, suffers from high transaction costs and limited throughput. Alternative platforms like EOSIO offer higher performance but lack EVM compatibility, creating ecosystem fragmentation. Zero-knowledge proof technologies promise scalability but often require sacrificing compatibility or developer experience.

Gambit addresses these challenges through a multi-pronged approach:

1. **Multi-VM Architecture**: Support for EVM, WASM, and native contracts in a unified state model
2. **Zero-Knowledge Consensus**: ZK validity proofs integrated into block validation
3. **Native Performance**: C++17 implementation for optimal resource utilization
4. **Ethereum Compatibility**: Full support for existing Ethereum tooling and standards

### 1.2 Core Innovations

- **Unified Multi-VM State**: Single state trie shared across all virtual machines
- **ZK Mining Engine**: Proof-of-work combined with zero-knowledge validity proofs
- **Cross-VM Calls**: Seamless interoperability between different contract types
- **Pluggable ZK Backend**: Extensible architecture for various ZK proof systems

## 2. System Architecture

### 2.1 Network Model

Gambit operates as a peer-to-peer network with the following components:

- **Nodes**: Full nodes maintaining complete blockchain state and validation logic
- **Miners**: Specialized nodes that produce blocks using the ZK mining engine
- **Light Clients**: Resource-constrained clients that verify blocks using ZK proofs
- **Seeders**: DNS-based bootstrap nodes for network discovery

### 2.2 Account Model

Gambit implements an Ethereum-compatible account model with the following structure:

```cpp
struct Account {
    uint64_t balance;    // Account balance in wei
    uint64_t nonce;      // Transaction nonce for replay protection
    // Future extensions: codeHash, storageRoot
};
```

Each account is identified by a 20-byte address derived from the secp256k1 public key using Ethereum's EIP-55 checksum encoding.

### 2.3 Transaction Model

Transactions follow Ethereum's EIP-155 standard with replay protection:

```cpp
struct Transaction {
    uint64_t nonce;      // Account nonce
    uint64_t gasPrice;   // Gas price in wei
    uint64_t gasLimit;   // Maximum gas allowed
    Address to;          // Recipient address
    uint64_t value;      // Transfer amount in wei
    bytes data;          // Transaction data/payload
    uint64_t chainId;    // Chain identifier (replay protection)
    Signature sig;       // ECDSA signature
};
```

### 2.4 State Management

Gambit employs a Merkle Patricia Tree (MPT) for state management, providing:

- **Deterministic State Transitions**: All state changes are cryptographically verifiable
- **Efficient Proofs**: Merkle proofs enable light client validation
- **Atomic Updates**: State transitions are atomic within blocks

## 3. Multi-Virtual Machine Architecture

### 3.1 Virtual Machine Types

Gambit supports three distinct virtual machine environments:

#### 3.1.1 EVM (Ethereum Virtual Machine)
- **Compatibility**: Full Ethereum EVM compatibility (opcodes 0x00-0xFF)
- **Gas Model**: Ethereum-compatible gas metering
- **Storage**: 256-bit storage slots in MPT
- **Use Case**: Solidity smart contracts and DeFi applications

#### 3.1.2 WASM (WebAssembly)
- **Runtime**: Deterministic WASM subset (no floating point, limited syscalls)
- **Host Functions**: Exposed blockchain APIs (balance queries, storage operations)
- **Performance**: Near-native execution speed
- **Use Case**: High-performance applications (gaming, complex computations)

#### 3.1.3 Core Contracts (Native)
- **Implementation**: Built-in C++ classes, not user-deployed code
- **Performance**: Maximum execution speed with minimal gas overhead
- **Security**: Audited native code
- **Use Case**: System contracts (token, governance, staking)

### 3.2 Contract Deployment

Contracts declare their VM type through a prefix byte in the deployment code:

- `0x01`: EVM bytecode
- `0x02`: WASM module
- `0x03`: Core contract reference

### 3.3 Cross-VM Communication

Gambit enables seamless calls between different VM types:

```cpp
// Example: EVM contract calling WASM contract
EVM → WASM: Direct bytecode call with data passing
WASM → EVM: Host function call_contract(address, data)
CORE → Any: Native dispatch to target VM
```

### 3.4 Unified State Model

All VMs share a single state trie with namespaced storage:

```
state[contractAddress][vmNamespace][key] = value

Examples:
- EVM: state[addr]["evm"][slot256] = value
- WASM: state[addr]["wasm"][keyBytes] = value
- CORE: state[addr]["core"][fieldName] = value
```

## 4. Zero-Knowledge Consensus

### 4.1 ZK Mining Engine

Gambit introduces a novel consensus mechanism combining proof-of-work with zero-knowledge validity proofs:

```cpp
Block ZkMiningEngine::buildBlockTemplate(Blockchain& chain) {
    string stateBefore = chain.state().root();
    // Apply transactions to temporary state
    State temp = chain.state();
    for (const auto& tx : mempool) {
        temp.applyTransaction(tx.from, tx);
    }
    string stateAfter = temp.root();
    string txRoot = chain.computeTxRoot(mempool);

    // Generate ZK proof of valid state transition
    ZkProof proof = ZkProver::generate(stateBefore, stateAfter, txRoot);

    return Block(index, prevHash, stateBefore, stateAfter, txRoot, proof);
}
```

### 4.2 ZK Proof Structure

```cpp
struct ZkProof {
    string stateBefore;    // Pre-state root
    string stateAfter;     // Post-state root
    string txRoot;         // Transaction Merkle root
    string proof;          // ZK proof data
    string commitment;     // Proof commitment
};
```

### 4.3 Proof Verification

Block validation requires both proof-of-work and ZK proof verification:

```cpp
bool ZkMiningEngine::validateMinedBlock(const Block& block, Blockchain& chain) {
    // Verify proof-of-work (traditional mining)
    // Verify ZK proof of state transition validity
    return ZkVerifier::verify(block.proof);
}
```

### 4.4 Pluggable ZK Backend

The ZK system is designed for extensibility:

- **Current**: Mock implementation using cryptographic hashing
- **Future**: Integration with production ZK systems (zk-SNARKs, zk-STARKs)
- **Modular**: Clean interface for swapping proof systems

## 5. Security Model

### 5.1 Cryptographic Foundations

- **Hash Function**: Keccak-256 (NIST SHA-3 finalist)
- **Digital Signatures**: secp256k1 ECDSA
- **Address Derivation**: EIP-55 checksummed addresses
- **Replay Protection**: EIP-155 chain ID inclusion

### 5.2 Consensus Security

- **Double-Spend Protection**: Account nonce mechanism
- **Transaction Ordering**: Deterministic block construction
- **State Consistency**: ZK validity proofs
- **Network Security**: Proof-of-work mining

### 5.3 VM Isolation

- **Sandboxing**: Each VM operates in isolated execution environment
- **Gas Limits**: Prevent infinite loops and resource exhaustion
- **Access Control**: Explicit permissions for cross-contract calls

## 6. Performance Characteristics

### 6.1 Benchmark Targets

- **Transaction Throughput**: 1,000+ TPS (baseline, scales with ZK optimization)
- **Block Time**: 15 seconds (configurable)
- **Finality**: Instant with ZK validity proofs
- **Storage**: MPT-based efficient state management

### 6.2 Optimization Strategies

- **Native C++ Implementation**: Maximum performance for critical paths
- **Memory Pool Management**: Efficient transaction processing
- **P2P Optimization**: Gossip protocol for block propagation
- **Light Client Support**: ZK proofs enable mobile/light verification

## 7. Token Economics

### 7.1 Gas Model

Gambit implements Ethereum-compatible gas economics:

- **Gas Price**: Market-determined transaction fees
- **Gas Limit**: Per-transaction computational bounds
- **Block Gas Limit**: Network-wide throughput control

### 7.2 Mining Rewards

- **Block Rewards**: Fixed rewards for successful mining
- **Transaction Fees**: Gas fees collected by miners
- **ZK Proof Incentives**: Additional rewards for ZK proof generation

### 7.3 Future Token Model

While currently operating without a native token, Gambit is designed to support:

- **Governance Token**: Community decision-making
- **Staking Mechanisms**: Validator incentives
- **Fee Markets**: Dynamic gas pricing

## 8. Development Roadmap

### 8.1 Phase 1: Core Implementation (Current)
- ✅ Multi-VM architecture
- ✅ ZK mining engine (mock implementation)
- ✅ P2P networking
- ✅ Wallet functionality
- ✅ Basic RPC interface

### 8.2 Phase 2: Production ZK Integration
- Production ZK proof system integration
- Light client optimization
- Cross-chain bridge protocols
- Advanced RPC APIs

### 8.3 Phase 3: Ecosystem Development
- IDE support and tooling
- Decentralized exchange integration
- Governance system implementation
- Staking and delegation mechanisms

### 8.4 Phase 4: Scaling Solutions
- Layer 2 protocols
- Sharding implementation
- Interoperability protocols
- Advanced ZK applications

## 9. Technical Specifications

### 9.1 Network Parameters

| Parameter | Value | Description |
|-----------|-------|-------------|
| Block Time | 15s | Target block production interval |
| Gas Limit | 10M | Maximum gas per block |
| Chain ID | 1 | Network identifier |
| Address Bytes | 20 | Address length in bytes |
| Hash Function | Keccak-256 | Cryptographic hash algorithm |

### 9.2 Protocol Versions

- **RLP Encoding**: Ethereum-compatible recursive length prefix
- **Transaction Format**: EIP-155 with replay protection
- **Block Structure**: Extended with ZK proof fields
- **P2P Protocol**: Custom binary protocol over TCP

### 9.3 RPC Interface

Gambit provides JSON-RPC 2.0 compatibility:

```json
{
  "jsonrpc": "2.0",
  "method": "eth_sendTransaction",
  "params": [{
    "from": "0x...",
    "to": "0x...",
    "value": "0x...",
    "data": "0x..."
  }],
  "id": 1
}
```

## 10. Conclusion

Gambit represents a significant advancement in blockchain technology by combining proven Ethereum compatibility with cutting-edge zero-knowledge proofs and multi-virtual-machine architecture. The platform's design enables:

- **Seamless Migration**: Existing Ethereum applications can run unmodified
- **Performance Scaling**: ZK proofs provide instant finality and light client efficiency
- **Developer Choice**: Multiple programming paradigms (Solidity, C++, Rust via WASM)
- **Future-Proofing**: Extensible architecture for emerging ZK technologies

By providing a unified platform for diverse computational models while maintaining cryptographic security and decentralization, Gambit establishes a foundation for the next generation of blockchain applications.

### 10.1 Vision

Gambit aims to become the "superchain" for decentralized applications, offering developers the flexibility to choose the most appropriate virtual machine for their use case while benefiting from shared liquidity, unified state, and cross-VM interoperability.

### 10.2 Call to Action

We invite developers, researchers, and blockchain enthusiasts to:

- Explore the codebase and contribute to development
- Build applications on the Gambit platform
- Research and implement advanced ZK proof systems
- Join the community discussion on future protocol enhancements

The future of scalable, interoperable blockchain platforms starts with Gambit.

---

*This whitepaper describes the Gambit blockchain platform as of December 2025. Specifications and features are subject to change through community governance and protocol upgrades.*
