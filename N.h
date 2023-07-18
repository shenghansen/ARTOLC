//
// Created by florian on 05.08.15.
//

#ifndef ART_OPTIMISTIC_LOCK_COUPLING_N_H
#define ART_OPTIMISTIC_LOCK_COUPLING_N_H
//#define ART_NOREADLOCK
//#define ART_NOWRITELOCK
#include <stdint.h>
#include <atomic>
#include <string.h>
#include "Key.h"
#include "Epoche.h"
#include <numa.h>

using TID = uint64_t;

using namespace ART;
namespace ART_OLC {
/*
 * SynchronizedTree
 * LockCouplingTree
 * LockCheckFreeReadTree
 * UnsynchronizedTree
 */

    const size_t NUMA_NODE = 4;

    enum class NTypes : uint8_t {
        N4 = 0,
        N16 = 1,
        N48 = 2,
        N256 = 3
    };

    static constexpr uint32_t maxStoredPrefixLength = 11;

    using Prefix = uint8_t[maxStoredPrefixLength];

    class N {
    protected:
        N(NTypes type, const uint8_t *prefix, uint32_t prefixLength) {
            setType(type);
            setPrefix(prefix, prefixLength);
        }

        //reload new
        void* operator new(size_t size, NTypes type, const uint8_t *prefix, uint32_t prefixLength){
            //printf("reload new N \n");
            void* n = numa_alloc_onnode(size, NUMA_NODE);
            if(n == nullptr){
                //printf("numa alloc failed \n");
            }else{
                //printf("%p\n", n);
                static_cast<N *>(n)->setType(type);
                static_cast<N *>(n)->setPrefix(prefix, prefixLength);
            }
            return n;
        }

        //reload delete
        void operator delete(void *p){
            //printf("numa free N \n");
            if(p==nullptr){
                //printf("numa free nullptr \n");
                return ;
            }
            numa_free(p, sizeof(N));
        }

        N(const N &) = delete;

        N(N &&) = delete;

        //2b type 60b version 1b lock 1b obsolete
        std::atomic<uint64_t> typeVersionLockObsolete{0b100};
        // version 1, unlocked, not obsolete
        uint32_t prefixCount = 0;

        uint8_t count = 0;
        Prefix prefix;


        void setType(NTypes type);

        static uint64_t convertTypeToVersion(NTypes type);

    public:

        NTypes getType() const;

        uint32_t getCount() const;

        bool isLocked(uint64_t version) const;

        void writeLockOrRestart(bool &needRestart);

        void upgradeToWriteLockOrRestart(uint64_t &version, bool &needRestart);

        void writeUnlock();

        uint64_t readLockOrRestart(bool &needRestart) const;

        /**
         * returns true if node hasn't been changed in between
         */
        void checkOrRestart(uint64_t startRead, bool &needRestart) const;
        void readUnlockOrRestart(uint64_t startRead, bool &needRestart) const;

        static bool isObsolete(uint64_t version);

        /**
         * can only be called when node is locked
         */
        void writeUnlockObsolete() {
            typeVersionLockObsolete.fetch_add(0b11);
        }

        static N *getChild(const uint8_t k, const N *node);

        static void insertAndUnlock(N *node, uint64_t v, N *parentNode, uint64_t parentVersion, uint8_t keyParent, uint8_t key, N *val, bool &needRestart,
                                    ThreadInfo &threadInfo);

        static bool change(N *node, uint8_t key, N *val);

        static void removeAndUnlock(N *node, uint64_t v, uint8_t key, N *parentNode, uint64_t parentVersion, uint8_t keyParent, bool &needRestart, ThreadInfo &threadInfo);

        bool hasPrefix() const;

        const uint8_t *getPrefix() const;

        void setPrefix(const uint8_t *prefix, uint32_t length);

        void addPrefixBefore(N *node, uint8_t key);

        uint32_t getPrefixLength() const;

        static TID getLeaf(const N *n);

        static bool isLeaf(const N *n);

        static N *setLeaf(TID tid);

        static N *getAnyChild(const N *n);

        static TID getAnyChildTid(const N *n, bool &needRestart);

        static void deleteChildren(N *node);

        static void deleteNode(N *node);

        static std::tuple<N *, uint8_t> getSecondChild(N *node, const uint8_t k);

        template<typename curN, typename biggerN>
        static void insertGrow(curN *n, uint64_t v, N *parentNode, uint64_t parentVersion, uint8_t keyParent, uint8_t key, N *val, bool &needRestart, ThreadInfo &threadInfo);

        template<typename curN, typename smallerN>
        static void removeAndShrink(curN *n, uint64_t v, N *parentNode, uint64_t parentVersion, uint8_t keyParent, uint8_t key, bool &needRestart, ThreadInfo &threadInfo);

        static uint64_t getChildren(const N *node, uint8_t start, uint8_t end, std::tuple<uint8_t, N *> children[],
                                uint32_t &childrenCount);
    };

    class N4 : public N {
    public:
        uint8_t keys[4];
        N *children[4] = {nullptr, nullptr, nullptr, nullptr};

    public:
        N4(const uint8_t *prefix, uint32_t prefixLength) : N(NTypes::N4, prefix,
                                                                             prefixLength) { }
        //reload new
        void* operator new(size_t size,const uint8_t *prefix, uint32_t prefixLength){
            //printf("reload new N4 \n");
            void* n = numa_alloc_onnode(size, NUMA_NODE);
            if(n == nullptr){
                //printf("numa alloc failed \n");
            }else{
                //printf("%p\n", n);
                // auto n4= static_cast<N4 *>(n);
                // n4->setType(NTypes::N4);
                // n4->setPrefix(prefix, prefixLength);
            }
            return n;
        }

        //reload delete
        void operator delete(void *p){
            //printf("numa free N4 \n");
            if(p==nullptr){
                //printf("numa free nullptr \n");
                return ;
            }
            numa_free(p, sizeof(N4));
        }   

        void insert(uint8_t key, N *n);

        template<class NODE>
        void copyTo(NODE *n) const;

        bool change(uint8_t key, N *val);

        N *getChild(const uint8_t k) const;

        void remove(uint8_t k);

        N *getAnyChild() const;

        bool isFull() const;

        bool isUnderfull() const;

        std::tuple<N *, uint8_t> getSecondChild(const uint8_t key) const;

        void deleteChildren();

        uint64_t getChildren(uint8_t start, uint8_t end, std::tuple<uint8_t, N *> *&children,
                         uint32_t &childrenCount) const;
    };

    class N16 : public N {
    public:
        uint8_t keys[16];
        N *children[16];

        static uint8_t flipSign(uint8_t keyByte) {
            // Flip the sign bit, enables signed SSE comparison of unsigned values, used by Node16
            return keyByte ^ 128;
        }

        static inline unsigned ctz(uint16_t x) {
            // Count trailing zeros, only defined for x>0
#ifdef __GNUC__
            return __builtin_ctz(x);
#else
            // Adapted from Hacker's Delight
   unsigned n=1;
   if ((x&0xFF)==0) {n+=8; x=x>>8;}
   if ((x&0x0F)==0) {n+=4; x=x>>4;}
   if ((x&0x03)==0) {n+=2; x=x>>2;}
   return n-(x&1);
#endif
        }

        N *const *getChildPos(const uint8_t k) const;

    public:
        N16(const uint8_t *prefix, uint32_t prefixLength) : N(NTypes::N16, prefix,
                                                                              prefixLength) {
            memset(keys, 0, sizeof(keys));
            memset(children, 0, sizeof(children));
        }

                //reload new
        void* operator new(size_t size,const uint8_t *prefix, uint32_t prefixLength){
            //printf("reload new N16 \n");
            void* n = numa_alloc_onnode(size, NUMA_NODE);
            if(n == nullptr){
                //printf("numa alloc failed \n");
            }else{
                //printf("%p\n", n);
                // auto n16 = static_cast<N16 *>(n);
                // n16->setType(NTypes::N16);
                // n16->setPrefix(prefix, prefixLength);
                // memset(n16->keys, 0, sizeof(n16->keys));
                // memset(n16->children, 0, sizeof(n16->children));
            }
            return n;
        }

        //reload delete
        void operator delete(void *p){
            //printf("numa free N16 \n");
            if(p==nullptr){
                //printf("numa free nullptr \n");
                return ;
            }
            numa_free(p, sizeof(N16));
        }   

        void insert(uint8_t key, N *n);

        template<class NODE>
        void copyTo(NODE *n) const;

        bool change(uint8_t key, N *val);

        N *getChild(const uint8_t k) const;

        void remove(uint8_t k);

        N *getAnyChild() const;

        bool isFull() const;

        bool isUnderfull() const;

        void deleteChildren();

        uint64_t getChildren(uint8_t start, uint8_t end, std::tuple<uint8_t, N *> *&children,
                         uint32_t &childrenCount) const;
    };

    class N48 : public N {
        uint8_t childIndex[256];
        N* children[48];
    public:
        static const uint8_t emptyMarker = 48;

        N48(const uint8_t *prefix, uint32_t prefixLength) : N(NTypes::N48, prefix,
                                                                              prefixLength) {
            memset(childIndex, emptyMarker, sizeof(childIndex));
            memset(children, 0, sizeof(children));
        }

        //reload new
        void* operator new(size_t size,const uint8_t *prefix, uint32_t prefixLength){
            //printf("reload new N48 \n");
            void* n = numa_alloc_onnode(size, NUMA_NODE);
            if(n == nullptr){
                //printf("numa alloc failed \n");
            }else{
                //printf("%p\n", n);
                // static_cast<N48 *>(n)->setType(NTypes::N48);
                // static_cast<N48 *>(n)->setPrefix(prefix, prefixLength);
            }
            return n;
        }

        //reload delete
        void operator delete(void *p){
            //printf("numa free N48\n");
            if(p==nullptr){
                //printf("numa free nullptr \n");
                return ;
            }
            numa_free(p, sizeof(N48));
        }   

        void insert(uint8_t key, N *n);

        template<class NODE>
        void copyTo(NODE *n) const;

        bool change(uint8_t key, N *val);

        N *getChild(const uint8_t k) const;

        void remove(uint8_t k);

        N *getAnyChild() const;

        bool isFull() const;

        bool isUnderfull() const;

        void deleteChildren();

        uint64_t getChildren(uint8_t start, uint8_t end, std::tuple<uint8_t, N *> *&children,
                         uint32_t &childrenCount) const;
    };

    class N256 : public N {
        N *children[256];

    public:
        N256(const uint8_t *prefix, uint32_t prefixLength) : N(NTypes::N256, prefix,
                                                                               prefixLength) {
            memset(children, '\0', sizeof(children));
        }

        //reload new
        void* operator new(size_t size,const uint8_t *prefix, uint32_t prefixLength){
            //printf("reload new N256 \n");
            void* n = numa_alloc_onnode(size, NUMA_NODE);
            if(n == nullptr){
                //printf("numa alloc failed \n");
            }else{
                //printf("%p,%ld\n", n,size);
                // static_cast<N256 *>(n)->setType(NTypes::N256);
                // static_cast<N256 *>(n)->setPrefix(prefix, prefixLength);
            }
            return n;
        }

        //reload delete
        void operator delete(void *p){
            //printf("numa free N256\n");
            if(p==nullptr){
                //printf("numa free nullptr \n");
                return ;
            }else{
                //printf("%p,%ld\n", p,sizeof(N256));
            }
            numa_free(p, sizeof(N256));
            //printf("free N256 over\n");
            // free(p);
        }   

        void insert(uint8_t key, N *val);

        template<class NODE>
        void copyTo(NODE *n) const;

        bool change(uint8_t key, N *n);

        N *getChild(const uint8_t k) const;

        void remove(uint8_t k);

        N *getAnyChild() const;

        bool isFull() const;

        bool isUnderfull() const;

        void deleteChildren();

        uint64_t getChildren(uint8_t start, uint8_t end, std::tuple<uint8_t, N *> *&children,
                         uint32_t &childrenCount) const;
    };
}
#endif //ART_OPTIMISTIC_LOCK_COUPLING_N_H
