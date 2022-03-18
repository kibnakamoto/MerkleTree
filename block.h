#include <iostream>
#include <vector>
#include <ctime>
#include <cmath>
#include <string.h>
#include <stdint.h>
#include <chrono>
#include <unistd.h>
#include <climits>
#include <algorithm>
#include <functional>

namespace Blockchain
{
    std::vector<std::string> blockchain;
    std::vector<uint64_t*> Blockhashes;
    
    inline std::string generateTimestamp()
    {
        std::time_t Time = std::time(nullptr);
        return std::asctime(std::localtime(&Time));
    }
    
    template<class T>
    inline T generateNonce()
    {
        /* random byte using Mersenne Twister. Not recommended for 
           cryptography but couldn't find a cryptographic random byte generator */
        std::random_device randDev;
        std::mt19937 generator(randDev() ^ time(NULL));
        std::uniform_int_distribution<T> distr;
        return distr(generator);
    }
    
    inline double difficulty(uint64_t nonce)
    {
        return 1;
    }
    
    /* hashes the bitcoin genesis block and adds to vector and length of vector is 
     * hashrate
     */
    inline uint64_t calchashRateSingle()
    {
        std::vector<std::string>hashes;
        auto start = std::chrono::system_clock::now();
        auto end_t = std::chrono::system_clock::now();
        do
        {
            std::string genesisBlockBtc =
            "GetHash()      = 0x000000000019d6689c085ae165831e934ff763ae46\
            a2a6c172b3f1b60a8ce26f\nhashMerkleRoot = 0x4a5e1e4baab89f3a3251\
            8a88c31bc87f618f76673e2cc77ab2127b7afdeda33b\ntxNew.vin[0].\
            scriptSig     = 486604799 4 0x736B6E616220726F662074756F6C69616\
            220646E6F63657320666F206B6E697262206E6F20726F6C6C65636E616843203\
            93030322F6E614A2F33302073656D695420656854\ntxNew.vout[0].nValue\
            = 5000000000\ntxNew.vout[0].scriptPubKey = 0x5F1DF16B2B704C8A57\
            8D0BBAF74D385CDE12C11EE50455F3C438EF4C3FBCF649B6DE611FEAE06279A\
            60939E028A8D65C10B73071A6F16719274855FEB0FD8A6704 OP_CHECKSIG\
            block.nVersion = 1\nblock.nTime    = 1231006505\nblock.nBits    \
            = 0x1d00ffff\nblock.nNonce   = 2083236893\nCBlock(hash=000000000\
            019d6, ver=1, hashPrevBlock=00000000000000, hashMerkleRoot=4a5e1\
            e, nTime=1231006505, nBits=1d00ffff, nNonce=2083236893, vtx=1)\n\
              CTransaction(hash=4a5e1e, ver=1, vin.size=1, vout.size=1, \
              nLockTime=0)\nCTxIn(COutPoint(000000, -1), coinbase 04ffff0\
              01d0104455468652054696d65732030332f4a616e2f32303039204368616\
              e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c\
              6f757420666f722062616e6b73)\nCTxOut(nValue=50.00000000, script\
              PubKey=0x5F1DF16B2B704C8A578D0B)\nvMerkleTree: 4a5e1e";
            hashes.push_back(sha512_str(genesisBlockBtc));
            end_t = std::chrono::system_clock::now();
        } while (std::chrono::duration_cast<std::chrono::seconds>
                 (end_t - start).count() != 1);
        return hashes.size();
    }
    
    inline uint64_t calcHashRateSha512(uint32_t accuracy=5)
    {
        std::vector<uint64_t> retvector;
        uint64_t ret=0;
        for(int c=0;c<accuracy;c++) {
            retvector.push_back(calchashRateSingle());
            ret += retvector[c];
        }
        ret/=accuracy;
        return ret;
    }
    
    inline double nextBlockTime(double difficulty,
                                uint64_t hashrate=calcHashRateSha512())
    {
        double timeM = difficulty * pow(2,32) / hashrate / 3600; // minutes
        return timeM;
    }
};

class PoW
{
    protected:
        std::pair<uint64_t*, bool> mineSingleTr(double difficulty, std::string 
                                                encryptedTr, uint8_t* key,
                                                uint64_t nonce, std::vector<uint64_t*> mempool)
        {
            AES::AES256 aes256;
            std::string transactionData = aes256.decrypt(encryptedTr, key);
            uint64_t* target = new uint64_t[8]; // each index >= 2^30
            bool valid;
            uint64_t newNonce = nonce;
            for(int c=0;c<8;c++) {
                while(target[c] > pow(2,30)) {
                    target[c] = sha512(transactionData + std::to_string(newNonce))[c];
                    newNonce++;
                }
            }
            
            // verify transaction data
            uint64_t* hash = new uint64_t[8];
            hash = sha512(transactionData);
            if(std::find(mempool.begin(), mempool.end(), hash) != mempool.end()) {
                valid = true;
            } else {
                valid = false;
            }
            return {target, valid};
        }
    public:
        bool mineBlock(const std::map<std::string, uint8_t*> encryptedTs,
                       uint64_t blockNonce, uint64_t difficulty,
                       std::vector<uint64_t*> mempool)
        {
            uint64_t loopt = 0;
            for (auto const& [key, val] : encryptedTs) {
                auto [trg,v] = mineSingleTr(difficulty, key, val, blockNonce,
                                            mempool);
                if(v == false) {
                    std::cout << "transaction hash mismatch, transaction index:\t"
                              << loopt << "\n";
                    mempool.erase(mempool.begin() + loopt);
                    std::cout << "transaction deleted from mempool";
                    loopt++; // mempool index
                    exit(EXIT_FAILURE);
                }
            }
            return true;
        }
};

class Block
{
    public:
        std::vector<uint64_t> hashrates;
        
        uint64_t averageHashRate()
        {
            uint64_t avHashrate = 0;
            for(int c=0;c<hashrates.size();c++) {
                avHashrate += hashrates[c];
            }
            return avHashrate;
        }
        
        std::string data(std::vector<uint64_t*> mempool, std::string
                         encryptedTr="", uint8_t* AESkey=nullptr)
        { // use this to access data from main file to mine
            SHA512 hash = SHA512();
            uint64_t* merkle_root = new uint64_t[8];
            MerkleTree::merkleRoot(mempool, merkle_root);
            MerkleTree::merkleRoots.push_back(merkle_root);
            uint64_t* prevBlockHash = new uint64_t[8];
            uint32_t blockchainsize = Blockchain::blockchain.size();
            std::string timestamp = Blockchain::generateTimestamp();
            uint64_t nonce = Blockchain::generateNonce<uint64_t>();
            uint64_t randHashNonce = Blockchain::generateNonce<uint64_t>();
            double difficulty = Blockchain::difficulty(randHashNonce);
            uint64_t hashrate = Blockchain::calcHashRateSha512(5);
            hashrates.push_back(hashrate); // put in different function in Node
            uint64_t avHashrate = averageHashRate();
            double blockGenTime = Blockchain::nextBlockTime(difficulty, avHashrate);
            std::cout << std::dec << blockGenTime << "\n\n";
            if(blockchainsize > 1) {
                prevBlockHash = Blockchain::Blockhashes[blockchainsize-1];
            }
            return std::string();
        }
};
