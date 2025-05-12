#ifndef CACHE_H
#define CACHE_H

#include <iostream>  // std::cout ...
#include <cstdlib>   // rand()

/*****************************************************************************/
/* Policy about L2 inclusion of L1's content                                 */
/*****************************************************************************/
#ifndef L2_INCLUSIVE
#  define L2_INCLUSIVE 1
#endif
/*****************************************************************************/


/*****************************************************************************/
/* Cache allocation strategy on stores                                       */
/*****************************************************************************/
enum {
    STORE_ALLOCATE = 0,
    STORE_NO_ALLOCATE
};
#ifndef STORE_ALLOCATION
#  define STORE_ALLOCATION STORE_ALLOCATE
#endif
/*****************************************************************************/

typedef UINT64 CACHE_STATS; // type of cache hit/miss counters


/**
 * `CACHE_TAG` class represents an address tag stored in a cache.
 * `INVALID_TAG` is used as an error on functions with CACHE_TAG return type.
 **/
class CACHE_TAG
{
  private:
    ADDRINT _tag;

  public:
    CACHE_TAG(ADDRINT tag = 0) : _tag(tag) {}
    bool operator==(const CACHE_TAG &right) const { return _tag == right._tag; }
    operator ADDRINT() const { return _tag; }
};
CACHE_TAG INVALID_TAG(-1);


/**
 * Everything related to cache sets
 **/
namespace CACHE_SET
{

// ************************
// Change this class and implement policy to evict random block!
// ************************

class LRU 
{
  protected:
    std::vector<CACHE_TAG> _tags;
    UINT32 _associativity;

 public:
    LRU(UINT32 associativity = 8)
    {
        _associativity = associativity;
        _tags.clear();
    }

    VOID SetAssociativity(UINT32 associativity)
    {
        _associativity = associativity;
        _tags.clear();
    }
    UINT32 GetAssociativity() { return _associativity; }

    string Name() { return "LRU"; }
    
    UINT32 Find(CACHE_TAG tag)
    {
        for (std::vector<CACHE_TAG>::iterator it = _tags.begin();
             it != _tags.end(); ++it)
        {
            if (*it == tag) { // Tag found, lets make it MRU
                _tags.erase(it);
                _tags.push_back(tag);
                return true;
            }
        }

        return false;
    }

    CACHE_TAG Replace(CACHE_TAG tag)
    {
        CACHE_TAG ret = INVALID_TAG;
        _tags.push_back(tag);
        if (_tags.size() > _associativity) {
            ret = *_tags.begin();
            _tags.erase(_tags.begin());
        }
        return ret;
    }
    
    VOID DeleteIfPresent(CACHE_TAG tag)
    {
        for (std::vector<CACHE_TAG>::iterator it = _tags.begin();
             it != _tags.end(); ++it)
        {
            if (*it == tag) { // Tag found
                _tags.erase(it);
                break;
            }
        }
    }
};


// ************************
// Random Replacement Policy (New implementation)
// ************************
class Random
{
  protected:
    std::vector<CACHE_TAG> _tags; // Stores the tags in the set
    UINT32 _associativity;

  public:
    // Constructor
    Random(UINT32 associativity = 8) : _associativity(associativity)
    {
        _tags.clear();
        _tags.reserve(associativity); // Reserve space for associativity
    }

    // Set associativity and clear existing tags
    VOID SetAssociativity(UINT32 associativity)
    {
        _associativity = associativity;
        _tags.clear();
        if (_associativity > 0) {
            _tags.reserve(associativity);
        }
    }

    // Get associativity
    UINT32 GetAssociativity() const { return _associativity; }

    // Return policy name
    std::string Name() const { return "Random"; }

    // Find a tag in the set
    // Returns true if found (hit), false otherwise (miss).
    // No reordering needed for Random policy on hit.
    bool Find(CACHE_TAG tag) const // Made const as it doesn't modify state
    {
        for (const auto& existing_tag : _tags) {
            if (existing_tag == tag) {
                return true; // Tag found (hit)
            }
        }
        return false; // Tag not found (miss)
    }

    // Replace a block in the set following Random policy.
    // Returns the evicted tag, or INVALID_TAG if no eviction occurred.
    CACHE_TAG Replace(CACHE_TAG tag)
    {
        CACHE_TAG evicted_tag = INVALID_TAG;

        if (_tags.size() < _associativity) {
            // Set is not full, just add the new tag
            _tags.push_back(tag);
        } else if (_associativity > 0) { // Check associativity > 0 to avoid modulo by zero
            // Set is full, need to replace a random tag
            // Generate a random index between 0 and associativity-1
            UINT32 victim_index = std::rand() % _associativity;

            // Store the tag being evicted
            evicted_tag = _tags[victim_index];

            // Replace the tag at the victim index with the new tag
            _tags[victim_index] = tag;
        }
        return evicted_tag;
    }

    // Delete a specific tag if it's present in the set (e.g., for L2 inclusivity)
    VOID DeleteIfPresent(CACHE_TAG tag)
    {
        // Iterate through the vector and erase the element if found
        for (auto it = _tags.begin(); it != _tags.end(); ++it) {
            if (*it == tag) { // Tag found
                _tags.erase(it); // Remove the tag
                // Assume tags are unique within a set for cache simulation, so we can stop.
                // If duplicates were possible, we'd continue or use std::remove/erase idiom.
                break;
            }
        }
    }
}; // End class Random


// ************************
// LFU (Least Frequently Used) Replacement Policy
// ************************
class LFU
{
  protected:
    // Δομή για την αποθήκευση του tag και της συχνότητας πρόσβασής του
    struct CacheEntry {
        CACHE_TAG tag;      // Το tag της γραμμής της cache
        UINT64 frequency;   // Μετρητής συχνότητας χρήσης (αριθμός hits)

        // Constructor για εύκολη δημιουργία (αρχική συχνότητα 1)
        CacheEntry(CACHE_TAG t, UINT64 f = 1) : tag(t), frequency(f) {}
    };
    std::vector<CacheEntry> _entries; // Αποθηκεύει τα entries (tag + frequency) του set
    UINT32 _associativity;            // Η συσχετιστικότητα του set

  public:
    // Constructor
    LFU(UINT32 associativity = 8) : _associativity(associativity)
    {
        // Προαιρετική δέσμευση χώρου στο vector για αποδοτικότητα
        if (_associativity > 0) {
             _entries.reserve(associativity);
        }
    }

    // Ορισμός συσχετιστικότητας και καθαρισμός των entries
    VOID SetAssociativity(UINT32 associativity)
    {
        _associativity = associativity;
        _entries.clear();
        if (_associativity > 0) {
             _entries.reserve(associativity);
        }
    }

    // Επιστροφή συσχετιστικότητας
    UINT32 GetAssociativity() const { return _associativity; }

    // Επιστροφή ονόματος πολιτικής
    std::string Name() const { return "LFU"; }

    // Εύρεση ενός tag στο set.
    // Επιστρέφει true αν βρεθεί (hit), false αλλιώς (miss).
    // **Σημαντικό**: Αυξάνει τον μετρητή frequency σε περίπτωση hit.
    bool Find(CACHE_TAG tag)
    {
        // Διατρέχουμε τα entries με αναφορά (&) για να μπορούμε να αλλάξουμε τη συχνότητα
        for (auto& entry : _entries) {
            if (entry.tag == tag) {
                entry.frequency++; // *** Αύξηση συχνότητας στο hit ***
                return true; // Το tag βρέθηκε
            }
        }
        return false; // Το tag δεν βρέθηκε
    }

    // Αντικατάσταση ενός block στο set ακολουθώντας την πολιτική LFU.
    // Επιστρέφει το tag που αφαιρέθηκε, ή INVALID_TAG αν δεν έγινε αφαίρεση.
    CACHE_TAG Replace(CACHE_TAG tag)
    {
        CACHE_TAG evicted_tag = INVALID_TAG;

        if (_entries.size() < _associativity) {
            // Το set δεν είναι γεμάτο. Απλά προσθέτουμε το νέο entry.
            // Χρησιμοποιούμε emplace_back για να κατασκευάσουμε το CacheEntry απευθείας μέσα στο vector.
            // Η αρχική συχνότητα είναι 1 (η τρέχουσα πρόσβαση).
            _entries.emplace_back(tag, 1);
        } else if (_associativity > 0) {
            // Το set είναι γεμάτο. Πρέπει να βρούμε και να αντικαταστήσουμε το LFU entry.

            // Βρίσκουμε τον δείκτη (index) του entry με τη μικρότερη συχνότητα.
            // Tie-breaking: Αν υπάρχουν πολλά με την ίδια ελάχιστη συχνότητα,
            // αυτή η υλοποίηση επιλέγει το *πρώτο* που θα συναντήσει στον vector.
            UINT64 min_freq = std::numeric_limits<UINT64>::max(); // Αρχικοποίηση με τη μέγιστη δυνατή τιμή
            UINT32 victim_index = 0; // Δείκτης του "θύματος" προς αντικατάσταση

            if (!_entries.empty()) { // Ασφάλεια για την περίπτωση assoc=0 ή άδειου vector
                 for (UINT32 i = 0; i < _entries.size(); ++i) {
                     if (_entries[i].frequency < min_freq) {
                         min_freq = _entries[i].frequency;
                         victim_index = i;
                     }
                 }

                 // Αποθηκεύουμε το tag του θύματος που θα αφαιρεθεί
                 evicted_tag = _entries[victim_index].tag;

                 // Αντικαθιστούμε το θύμα με το νέο tag και **μηδενίζουμε/αρχικοποιούμε** τη συχνότητά του.
                 // Η νέα συχνότητα τίθεται σε 1, γιατί αυτή η εισαγωγή μετράει ως η πρώτη του χρήση.
                 _entries[victim_index].tag = tag;
                 _entries[victim_index].frequency = 1;
            }
        }
        return evicted_tag;
    }

    // Διαγραφή ενός συγκεκριμένου tag αν υπάρχει στο set (π.χ., για την L2 inclusivity)
    VOID DeleteIfPresent(CACHE_TAG tag)
    {
        for (auto it = _entries.begin(); it != _entries.end(); ++it) {
            // it->tag για πρόσβαση στο μέλος tag του struct CacheEntry
            if (it->tag == tag) { // Βρέθηκε το tag
                _entries.erase(it); // Αφαιρούμε το στοιχείο CacheEntry
                break;              // Υποθέτουμε μοναδικότητα των tags
            }
        }
    }
}; // End class LFU


// ************************
// LIP (LRU Insertion Policy) Replacement Policy
// ************************
class LIP
{
  protected:
    std::vector<CACHE_TAG> _tags; // Αποθηκεύει τα tags. front()=LRU, back()=MRU
    UINT32 _associativity;        // Η συσχετιστικότητα του set

  public:
    // Constructor
    LIP(UINT32 associativity = 8) : _associativity(associativity)
    {
        // Προαιρετική δέσμευση χώρου στο vector για αποδοτικότητα
        if (_associativity > 0) {
            _tags.reserve(associativity);
        }
    }

    // Ορισμός συσχετιστικότητας και καθαρισμός των tags
    VOID SetAssociativity(UINT32 associativity)
    {
        _associativity = associativity;
        _tags.clear();
        if (_associativity > 0) {
            _tags.reserve(associativity);
        }
    }

    // Επιστροφή συσχετιστικότητας
    UINT32 GetAssociativity() const { return _associativity; }

    // Επιστροφή ονόματος πολιτικής
    std::string Name() const { return "LIP"; }

    // Εύρεση ενός tag στο set.
    // Επιστρέφει true αν βρεθεί (hit), false αλλιώς (miss).
    // **Σε περίπτωση hit, προωθεί το tag στη θέση MRU (όπως η LRU).**
    bool Find(CACHE_TAG tag)
    {
        for (auto it = _tags.begin(); it != _tags.end(); ++it) {
            if (*it == tag) { // Το tag βρέθηκε (Hit)
                _tags.erase(it);           // Αφαιρούμε το tag από την τρέχουσα θέση του
                _tags.push_back(tag);// Το προσθέτουμε στο τέλος (το κάνουμε MRU)
                return true;
            }
        }
        return false; // Το tag δεν βρέθηκε (Miss)
    }

    // Αντικατάσταση ενός block στο set ακολουθώντας την πολιτική LIP.
    // Επιστρέφει το tag που αφαιρέθηκε, ή INVALID_TAG αν δεν έγινε αφαίρεση.
    // **Σημαντική διαφορά από LRU: Εισάγει το νέο tag στη θέση LRU.**
    CACHE_TAG Replace(CACHE_TAG tag)
    {
        CACHE_TAG evicted_tag = INVALID_TAG;

        // Έλεγχος αν το set είναι γεμάτο (και η associativity > 0)
        if (_tags.size() >= _associativity && _associativity > 0) {
            // Το set είναι γεμάτο. Πρέπει να αφαιρέσουμε το LRU block (αυτό που είναι στην αρχή).
            evicted_tag = _tags.front(); // Παίρνουμε το tag του LRU
            _tags.erase(_tags.begin());  // Αφαιρούμε το LRU στοιχείο
        }
        // Αν associativity == 0, δεν κάνουμε τίποτα εδώ.

        // Εισάγουμε το νέο tag στην αρχή του vector (στη θέση LRU).
        // Αυτό γίνεται είτε το set ήταν γεμάτο (και μόλις αδειάσαμε μια θέση) είτε όχι.
        if (_associativity > 0) { // Εισάγουμε μόνο αν η cache μπορεί να κρατήσει στοιχεία
            _tags.insert(_tags.begin(), tag);
        }

        // Επιστρέφουμε το tag που (πιθανώς) αφαιρέθηκε
        return evicted_tag;
    }

    // Διαγραφή ενός συγκεκριμένου tag αν υπάρχει στο set (ίδιο με την LRU)
    VOID DeleteIfPresent(CACHE_TAG tag)
    {
        for (auto it = _tags.begin(); it != _tags.end(); ++it) {
            if (*it == tag) { // Βρέθηκε το tag
                _tags.erase(it); // Το αφαιρούμε
                break;           // Υποθέτουμε μοναδικότητα των tags
            }
        }
    }
}; // End class LIP

// ************************
// SRRIP (Static Re-reference Interval Prediction) Replacement Policy
// ************************
class SRRIP
{
  protected:
    // Δομή για την αποθήκευση του tag και της τιμής RRPV
    struct CacheEntry {
        CACHE_TAG tag;  // Το tag της γραμμής
        UINT64 rrpv;    // Re-Reference Prediction Value (χρησιμοποιούμε UINT64 για ασφάλεια αν n είναι μεγάλο)

        // Constructor
        CacheEntry(CACHE_TAG t, UINT64 r) : tag(t), rrpv(r) {}
    };

    std::vector<CacheEntry> _entries; // Αποθηκεύει τα entries (tag + RRPV)
    UINT32 _associativity;            // Η συσχετιστικότητα (n)
    UINT64 _rmax;                     // Η μέγιστη τιμή RRPV (Rmax = 2^n - 1)

    // Βοηθητική συνάρτηση για τον υπολογισμό του Rmax = 2^n - 1
    // Χρησιμοποιεί bit shift (1ULL << n) που είναι πιο ασφαλές/αποδοτικό από pow() για ακέραιες δυνάμεις του 2.
    static UINT64 calculate_rmax(UINT32 associativity) {
        if (associativity == 0) return 0;
        // Προσοχή σε πολύ μεγάλες τιμές associativity (αν n >= 64, το shift θα υπερχειλίσει το UINT64)
        if (associativity >= 64) {
            // Σε αυτή την απίθανη περίπτωση, επιστρέφουμε τη μέγιστη τιμή UINT64
            return std::numeric_limits<UINT64>::max();
        }
        // Υπολογισμός 2^n - 1 χρησιμοποιώντας bit shift
        return (1ULL << associativity) - 1;
    }

  public:
    // Constructor
    SRRIP(UINT32 associativity = 8) : _associativity(associativity)
    {
        _rmax = calculate_rmax(_associativity);
        // Προαιρετική δέσμευση χώρου
        if (_associativity > 0) {
             _entries.reserve(associativity);
        }
    }

    // Ορισμός συσχετιστικότητας, υπολογισμός Rmax και καθαρισμός
    VOID SetAssociativity(UINT32 associativity)
    {
        _associativity = associativity;
        _rmax = calculate_rmax(_associativity); // Επαναϋπολογισμός Rmax
        _entries.clear();
        if (_associativity > 0) {
             _entries.reserve(associativity);
        }
    }

    // Επιστροφή συσχετιστικότητας
    UINT32 GetAssociativity() const { return _associativity; }

    // Επιστροφή ονόματος πολιτικής
    std::string Name() const { return "SRRIP"; }

    // Εύρεση ενός tag στο set.
    // Επιστρέφει true αν βρεθεί (hit), false αλλιώς (miss).
    // **Σε περίπτωση hit, θέτει το RRPV του tag σε 0.**
    bool Find(CACHE_TAG tag)
    {
        // Διατρέχουμε με αναφορά (&) για να αλλάξουμε το rrpv
        for (auto& entry : _entries) {
            if (entry.tag == tag) {
                entry.rrpv = 0; // Θέτουμε RRPV=0 στο hit
                return true;    // Το tag βρέθηκε
            }
        }
        return false; // Το tag δεν βρέθηκε
    }

    // Αντικατάσταση ενός block στο set ακολουθώντας την πολιτική SRRIP.
    // Επιστρέφει το tag που αφαιρέθηκε, ή INVALID_TAG αν δεν έγινε αφαίρεση.
    CACHE_TAG Replace(CACHE_TAG tag)
    {
        CACHE_TAG evicted_tag = INVALID_TAG;
        // Αρχική τιμή RRPV για νέα blocks (Rmax-1, ή 0 αν Rmax=0)
        UINT64 initial_rrpv = (_rmax > 0) ? (_rmax - 1) : 0;

        // Έλεγχος αν το set είναι γεμάτο
        if (_entries.size() < _associativity) {
            // Το set δεν είναι γεμάτο, απλά προσθέτουμε το νέο entry με RRPV = Rmax-1.
            _entries.emplace_back(tag, initial_rrpv);
        }
        // Έλεγχος αν το set είναι γεμάτο (και η associativity > 0)
        else if (_associativity > 0) {
            // Το set είναι γεμάτο. Πρέπει να βρούμε θύμα με RRPV == Rmax.

            UINT32 victim_index = 0;    // Ο δείκτης του θύματος
            bool victim_found = false;  // Flag για να ξέρουμε αν βρήκαμε θύμα

            // Βρόχος αναζήτησης θύματος: συνεχίζει μέχρι να βρεθεί ένα με RRPV == Rmax
            while (!victim_found) {
                 // Πρώτη προσπάθεια: βρες ένα entry με RRPV == Rmax
                 for (UINT32 i = 0; i < _entries.size(); ++i) {
                     if (_entries[i].rrpv == _rmax) {
                         victim_index = i;   // Βρήκαμε θύμα
                         victim_found = true;
                         break; // Έξοδος από τον εσωτερικό βρόχο for
                     }
                 }

                 // Αν βρήκαμε θύμα σε αυτή την επανάληψη, βγαίνουμε και από τον εξωτερικό βρόχο while
                 if (victim_found) {
                     break;
                 }

                 // Αν *δεν* βρέθηκε θύμα με RRPV == Rmax σε αυτή την επανάληψη,
                 // αυξάνουμε *όλα* τα RRPV κατά 1 και ξαναψάχνουμε στον επόμενο κύκλο του while.
                 for (auto& entry : _entries) {
                     // Αυξάνουμε το RRPV. Η λογική του βρόχου while διασφαλίζει ότι
                     // δεν θα μείνουμε εδώ για πάντα και ότι τελικά κάποιο θα φτάσει Rmax.
                     // Δεν χρειάζεται έλεγχος υπερχείλισης εδώ γιατί ο βρόχος θα τερματίσει.
                     entry.rrpv++;
                 }
            } // Τέλος βρόχου while (!victim_found)

            // Έχουμε βρει το θύμα στον δείκτη victim_index
            // Αποθηκεύουμε το tag του θύματος
            evicted_tag = _entries[victim_index].tag;

            // Αντικαθιστούμε το θύμα με το νέο tag και αρχικοποιούμε το RRPV του νέου tag
            _entries[victim_index].tag = tag;
            _entries[victim_index].rrpv = initial_rrpv; // RRPV = Rmax - 1 για το νέο tag
        }
        // Αν associativity == 0, δεν κάνουμε τίποτα.

        // Επιστρέφουμε το tag που (πιθανώς) αφαιρέθηκε
        return evicted_tag;
    }

    // Διαγραφή ενός συγκεκριμένου tag αν υπάρχει στο set (π.χ., για την L2 inclusivity)
    VOID DeleteIfPresent(CACHE_TAG tag)
    {
        for (auto it = _entries.begin(); it != _entries.end(); ++it) {
            if (it->tag == tag) { // Βρέθηκε το tag
                _entries.erase(it); // Αφαιρούμε το στοιχείο CacheEntry
                break;              // Υποθέτουμε μοναδικότητα των tags
            }
        }
    }
}; // End class SRRIP


} // namespace CACHE_SET

template <class SET>
class TWO_LEVEL_CACHE
{
  public:
    typedef enum 
    {
        ACCESS_TYPE_LOAD,
        ACCESS_TYPE_STORE,
        ACCESS_TYPE_NUM
    } ACCESS_TYPE;

  private:
    enum {
        HIT_L1 = 0,
        HIT_L2,
        MISS_L2,
        ACCESS_RESULT_NUM
    };

    static const UINT32 HIT_MISS_NUM = 2;
    CACHE_STATS _l1_access[ACCESS_TYPE_NUM][HIT_MISS_NUM];
    CACHE_STATS _l2_access[ACCESS_TYPE_NUM][HIT_MISS_NUM];

    UINT32 _latencies[ACCESS_RESULT_NUM];

    SET *_l1_sets;
    SET *_l2_sets;

    const std::string _name;
    const UINT32 _l1_cacheSize;
    const UINT32 _l2_cacheSize;
    const UINT32 _l1_blockSize;
    const UINT32 _l2_blockSize;
    const UINT32 _l1_associativity;
    const UINT32 _l2_associativity;

    // computed params
    const UINT32 _l1_lineShift; // i.e., no of block offset bits
    const UINT32 _l2_lineShift;
    const UINT32 _l1_setIndexMask; // mask applied to get the set index
    const UINT32 _l2_setIndexMask;

    // how many lines ahead to prefetch in L2 (0 disables prefetching)
    const UINT32 _l2_prefetch_lines;

    CACHE_STATS L1SumAccess(bool hit) const
    {
        CACHE_STATS sum = 0;
        for (UINT32 accessType = 0; accessType < ACCESS_TYPE_NUM; accessType++)
            sum += _l1_access[accessType][hit];
        return sum;
    }
    CACHE_STATS L2SumAccess(bool hit) const
    {
        CACHE_STATS sum = 0;
        for (UINT32 accessType = 0; accessType < ACCESS_TYPE_NUM; accessType++)
            sum += _l2_access[accessType][hit];
        return sum;
    }

    UINT32 L1NumSets() const { return _l1_setIndexMask + 1; }
    UINT32 L2NumSets() const { return _l2_setIndexMask + 1; }

    // accessors
    UINT32 L1CacheSize() const { return _l1_cacheSize; }
    UINT32 L2CacheSize() const { return _l2_cacheSize; }
    UINT32 L1BlockSize() const { return _l1_blockSize; }
    UINT32 L2BlockSize() const { return _l2_blockSize; }
    UINT32 L1Associativity() const { return _l1_associativity; }
    UINT32 L2Associativity() const { return _l2_associativity; }
    UINT32 L1LineShift() const { return _l1_lineShift; }
    UINT32 L2LineShift() const { return _l2_lineShift; }
    UINT32 L1SetIndexMask() const { return _l1_setIndexMask; }
    UINT32 L2SetIndexMask() const { return _l2_setIndexMask; }

    VOID SplitAddress(const ADDRINT addr, UINT32 lineShift, UINT32 setIndexMask,
                      CACHE_TAG & tag, UINT32 & setIndex) const
    {
        tag = addr >> lineShift;
        setIndex = tag & setIndexMask;
        tag = tag >> FloorLog2(setIndexMask + 1);
    }


  public:
    // constructors/destructors
    TWO_LEVEL_CACHE(std::string name,
                UINT32 l1CacheSize, UINT32 l1BlockSize, UINT32 l1Associativity,
                UINT32 l2CacheSize, UINT32 l2BlockSize, UINT32 l2Associativity,
                UINT32 l2PrefetchLines,
                UINT32 l1HitLatency = 1, UINT32 l2HitLatency = 15,
                UINT32 l2MissLatency = 250);

    // Stats
    CACHE_STATS L1Hits(ACCESS_TYPE accessType) const { return _l1_access[accessType][true];}
    CACHE_STATS L2Hits(ACCESS_TYPE accessType) const { return _l2_access[accessType][true];}
    CACHE_STATS L1Misses(ACCESS_TYPE accessType) const { return _l1_access[accessType][false];}
    CACHE_STATS L2Misses(ACCESS_TYPE accessType) const { return _l2_access[accessType][false];}
    CACHE_STATS L1Accesses(ACCESS_TYPE accessType) const { return L1Hits(accessType) + L1Misses(accessType);}
    CACHE_STATS L2Accesses(ACCESS_TYPE accessType) const { return L2Hits(accessType) + L2Misses(accessType);}
    CACHE_STATS L1Hits() const { return L1SumAccess(true);}
    CACHE_STATS L2Hits() const { return L2SumAccess(true);}
    CACHE_STATS L1Misses() const { return L1SumAccess(false);}
    CACHE_STATS L2Misses() const { return L2SumAccess(false);}
    CACHE_STATS L1Accesses() const { return L1Hits() + L1Misses();}
    CACHE_STATS L2Accesses() const { return L2Hits() + L2Misses();}

    string StatsLong(string prefix = "") const;
    string PrintCache(string prefix = "") const;

    UINT32 Access(ADDRINT addr, ACCESS_TYPE accessType);
};

template <class SET>
TWO_LEVEL_CACHE<SET>::TWO_LEVEL_CACHE(
                std::string name,
                UINT32 l1CacheSize, UINT32 l1BlockSize, UINT32 l1Associativity,
                UINT32 l2CacheSize, UINT32 l2BlockSize, UINT32 l2Associativity,
                UINT32 l2PrefetchLines,
                UINT32 l1HitLatency, UINT32 l2HitLatency, UINT32 l2MissLatency)
  : _name(name),
    _l1_cacheSize(l1CacheSize),
    _l2_cacheSize(l2CacheSize),
    _l1_blockSize(l1BlockSize),
    _l2_blockSize(l2BlockSize),
    _l1_associativity(l1Associativity),
    _l2_associativity(l2Associativity),
    _l1_lineShift(FloorLog2(l1BlockSize)),
    _l2_lineShift(FloorLog2(l2BlockSize)),
    _l1_setIndexMask((l1CacheSize / (l1Associativity * l1BlockSize)) - 1),
    _l2_setIndexMask((l2CacheSize / (l2Associativity * l2BlockSize)) - 1),
    _l2_prefetch_lines(l2PrefetchLines)
{

    // They all need to be power of 2
    ASSERTX(IsPowerOf2(_l1_blockSize));
    ASSERTX(IsPowerOf2(_l2_blockSize));
    ASSERTX(IsPowerOf2(_l1_setIndexMask + 1));
    ASSERTX(IsPowerOf2(_l2_setIndexMask + 1));

    // Some more sanity checks
    ASSERTX(_l1_cacheSize <= _l2_cacheSize);
    ASSERTX(_l1_blockSize <= _l2_blockSize);

    // Allocate space for L1 and L2 sets
    _l1_sets = new SET[L1NumSets()];
    _l2_sets = new SET[L2NumSets()];

    _latencies[HIT_L1] = l1HitLatency;
    _latencies[HIT_L2] = l2HitLatency;
    _latencies[MISS_L2] = l2MissLatency;

    for (UINT32 i = 0; i < L1NumSets(); i++)
        _l1_sets[i].SetAssociativity(_l1_associativity);
    for (UINT32 i = 0; i < L2NumSets(); i++)
        _l2_sets[i].SetAssociativity(_l2_associativity);

    for (UINT32 accessType = 0; accessType < ACCESS_TYPE_NUM; accessType++)
    {
        _l1_access[accessType][false] = 0;
        _l1_access[accessType][true] = 0;
        _l2_access[accessType][false] = 0;
        _l2_access[accessType][true] = 0;
    }
}

template <class SET>
string TWO_LEVEL_CACHE<SET>::StatsLong(string prefix) const
{
    const UINT32 headerWidth = 19;
    const UINT32 numberWidth = 12;

    string out;
    
    // L1 stats first
    out += prefix + "L1 Cache Stats:" + "\n";

    for (UINT32 i = 0; i < ACCESS_TYPE_NUM; i++)
    {
        const ACCESS_TYPE accessType = ACCESS_TYPE(i);

        std::string type(accessType == ACCESS_TYPE_LOAD ? "L1-Load" : "L1-Store");

        out += prefix + ljstr(type + "-Hits:      ", headerWidth)
               + dec2str(L1Hits(accessType), numberWidth)  +
               "  " +fltstr(100.0 * L1Hits(accessType) / L1Accesses(accessType), 2, 6) + "%\n";

        out += prefix + ljstr(type + "-Misses:    ", headerWidth)
               + dec2str(L1Misses(accessType), numberWidth) +
               "  " +fltstr(100.0 * L1Misses(accessType) / L1Accesses(accessType), 2, 6) + "%\n";
     
        out += prefix + ljstr(type + "-Accesses:  ", headerWidth)
               + dec2str(L1Accesses(accessType), numberWidth) +
               "  " +fltstr(100.0 * L1Accesses(accessType) / L1Accesses(accessType), 2, 6) + "%\n";
     
        out += prefix + "\n";
    }

    out += prefix + ljstr("L1-Total-Hits:      ", headerWidth)
           + dec2str(L1Hits(), numberWidth) +
           "  " +fltstr(100.0 * L1Hits() / L1Accesses(), 2, 6) + "%\n";

    out += prefix + ljstr("L1-Total-Misses:    ", headerWidth)
           + dec2str(L1Misses(), numberWidth) +
           "  " +fltstr(100.0 * L1Misses() / L1Accesses(), 2, 6) + "%\n";

    out += prefix + ljstr("L1-Total-Accesses:  ", headerWidth)
           + dec2str(L1Accesses(), numberWidth) +
           "  " +fltstr(100.0 * L1Accesses() / L1Accesses(), 2, 6) + "%\n";
    out += "\n";


    // L2 Stats now.
    out += prefix + "L2 Cache Stats:" + "\n";

    for (UINT32 i = 0; i < ACCESS_TYPE_NUM; i++)
    {
        const ACCESS_TYPE accessType = ACCESS_TYPE(i);

        std::string type(accessType == ACCESS_TYPE_LOAD ? "L2-Load" : "L2-Store");

        out += prefix + ljstr(type + "-Hits:      ", headerWidth)
               + dec2str(L2Hits(accessType), numberWidth)  +
               "  " +fltstr(100.0 * L2Hits(accessType) / L2Accesses(accessType), 2, 6) + "%\n";

        out += prefix + ljstr(type + "-Misses:    ", headerWidth)
               + dec2str(L2Misses(accessType), numberWidth) +
               "  " +fltstr(100.0 * L2Misses(accessType) / L2Accesses(accessType), 2, 6) + "%\n";
     
        out += prefix + ljstr(type + "-Accesses:  ", headerWidth)
               + dec2str(L2Accesses(accessType), numberWidth) +
               "  " +fltstr(100.0 * L2Accesses(accessType) / L2Accesses(accessType), 2, 6) + "%\n";
     
        out += prefix + "\n";
    }

    out += prefix + ljstr("L2-Total-Hits:      ", headerWidth)
           + dec2str(L2Hits(), numberWidth) +
           "  " +fltstr(100.0 * L2Hits() / L2Accesses(), 2, 6) + "%\n";

    out += prefix + ljstr("L2-Total-Misses:    ", headerWidth)
           + dec2str(L2Misses(), numberWidth) +
           "  " +fltstr(100.0 * L2Misses() / L2Accesses(), 2, 6) + "%\n";

    out += prefix + ljstr("L2-Total-Accesses:  ", headerWidth)
           + dec2str(L2Accesses(), numberWidth) +
           "  " +fltstr(100.0 * L2Accesses() / L2Accesses(), 2, 6) + "%\n";
    out += prefix + "\n";

    return out;
}

template <class SET>
string TWO_LEVEL_CACHE<SET>::PrintCache(string prefix) const
{
    string out;

    out += prefix + "--------\n";
    out += prefix + _name + "\n";
    out += prefix + "--------\n";
    out += prefix + "  L1-Data Cache:\n";
    out += prefix + "    Size(KB):       " + dec2str(this->L1CacheSize()/KILO, 5) + "\n";
    out += prefix + "    Block Size(B):  " + dec2str(this->L1BlockSize(), 5) + "\n";
    out += prefix + "    Associativity:  " + dec2str(this->L1Associativity(), 5) + "\n";
    out += prefix + "\n";
    out += prefix + "  L2-Data Cache:\n";
    out += prefix + "    Size(KB):       " + dec2str(this->L2CacheSize()/KILO, 5) + "\n";
    out += prefix + "    Block Size(B):  " + dec2str(this->L2BlockSize(), 5) + "\n";
    out += prefix + "    Associativity:  " + dec2str(this->L2Associativity(), 5) + "\n";
    out += prefix + "\n";

    out += prefix + "Latencies: " + dec2str(_latencies[HIT_L1], 4) + " "
                                  + dec2str(_latencies[HIT_L2], 4) + " "
                                  + dec2str(_latencies[MISS_L2], 4) + "\n";
    //out += prefix + "L1-Sets: " + this->_l1_sets[0].Name() + " assoc: " +
    out += prefix + "L1-Sets: " + dec2str(this->L1NumSets(), 4) + " - " + this->_l1_sets[0].Name() + " - assoc: " +
                          dec2str(this->_l1_sets[0].GetAssociativity(), 3) + "\n";
    //out += prefix + "L2-Sets: " + this->_l2_sets[0].Name() + " assoc: " +
    out += prefix + "L2-Sets: " + dec2str(this->L2NumSets(), 4) + " - " + this->_l2_sets[0].Name() + " - assoc: " +
                          dec2str(this->_l2_sets[0].GetAssociativity(), 3) + "\n";
    out += prefix + "Store_allocation: " + (STORE_ALLOCATION == STORE_ALLOCATE ? "Yes" : "No") + "\n";
    out += prefix + "L2_inclusive: " + (L2_INCLUSIVE == 1 ? "Yes" : "No") + "\n";
//    out += prefix + "L2_prefetching: " + (_l2_prefetch_lines <= 0 ? "No" : "Yes (" + dec2str(_l2_prefetch_lines, 3) + ")") + "\n";
    out += "\n";

    return out;
}

// Returns the cycles to serve the request.
template <class SET>
UINT32 TWO_LEVEL_CACHE<SET>::Access(ADDRINT addr, ACCESS_TYPE accessType)
{
    CACHE_TAG l1Tag, l2Tag;
    UINT32 l1SetIndex, l2SetIndex;
    bool l1Hit = 0, l2Hit = 0;
    UINT32 cycles = 0;

    // Let's check L1 first
    SplitAddress(addr, L1LineShift(), L1SetIndexMask(), l1Tag, l1SetIndex);
    SET & l1Set = _l1_sets[l1SetIndex];
    l1Hit = l1Set.Find(l1Tag);
    _l1_access[accessType][l1Hit]++;
    cycles = _latencies[HIT_L1];

    if (!l1Hit) {
        // On miss, loads always allocate, stores optionally
        if (accessType == ACCESS_TYPE_LOAD ||
            STORE_ALLOCATION == STORE_ALLOCATE)
            l1Set.Replace(l1Tag);

        // Let's check L2 now
        SplitAddress(addr, L2LineShift(), L2SetIndexMask(), l2Tag, l2SetIndex);
        SET & l2Set = _l2_sets[l2SetIndex];
        l2Hit = l2Set.Find(l2Tag);
        _l2_access[accessType][l2Hit]++;
        cycles += _latencies[HIT_L2];

        // L2 always allocates loads and stores
        if (!l2Hit) {
            CACHE_TAG l2_replaced = l2Set.Replace(l2Tag);
            cycles += _latencies[MISS_L2];

            // If L2 is inclusive and a TAG has been replaced we need to remove
            // all evicted blocks from L1.
            if ((L2_INCLUSIVE == 1) && !(l2_replaced == INVALID_TAG)) {
                ADDRINT replacedAddr = ADDRINT(l2_replaced) << FloorLog2(L2NumSets());
                replacedAddr = replacedAddr | l2SetIndex;
                replacedAddr = replacedAddr << L2LineShift();
                for (UINT32 i=0; i < L2BlockSize(); i+=L1BlockSize()) {
                    ADDRINT newAddr = replacedAddr | i;
                    SplitAddress(newAddr, L1LineShift(), L1SetIndexMask(), l1Tag, l1SetIndex);
                    SET & l1Set = _l1_sets[l1SetIndex];
                    l1Set.DeleteIfPresent(l1Tag);
                }
            }
	}

    }

    return cycles;
}

#endif // CACHE_H
