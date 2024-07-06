
#include "comparator.h"
#include <assert.h>
#undef min
#undef max
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))
sds bytewiseComparatorName() {
    static sds name = NULL;
    if (name == NULL) {
        name = sdsnew("leveldb.BytewiseComparator");
    }
    return name;
}

int bytewiseComparatorCompare(sds a, sds b) {
    size_t alen = sdslen(a);
    size_t blen = sdslen(b);
    const size_t min_len = min(alen , blen);
    int r = memcmp(a, b, min_len);
    if (r == 0) {
        if (alen < blen) {
            r = -1;
        } else if (alen > blen) {
            r = +1;
        }
    } 
    return r;
}
void bytewiseComparatorFindShortestSeparator(sds* start,
                                    sds limit) {
    size_t min_length = min(sdslen(*start), sdslen(limit));
    size_t diff_index = 0;
    while ((diff_index < min_length) &&
           ((*start)[diff_index] == limit[diff_index])) {
      diff_index++;
    }
    
    if (diff_index >= min_length) {
      // Do not shorten if one string is a prefix of the other
      //如果前缀一致  不压缩
    } else {
       
      //读取不相同的字符
      uint8_t diff_byte =(*start)[diff_index];
      //如果字符 <0xff 而且  +1 要小于limit相同位置字符 才进行修改 字符+1 调整长度
      //start（abcd）和   limit（abdc） 不改变
      //start （abcd）和  limit（abec）  ，abcd=> abd
      if (diff_byte < (0xff) &&
          diff_byte + 1 <  (limit[diff_index])) {
        (*start)[diff_index]++;
        sdsResize(*start, diff_index + 1, 1);
        assert(bytewiseComparatorCompare(*start, limit) < 0);
      }
    }
}

void bytewiseComparatorFindShortSuccessor(sds* key) {
    size_t n = sdslen(*key);
    for (size_t i = 0; i < n; i++) {
        const uint8_t byte = (*key)[i];
        //找到第一个非0xff的字符  然后进行+1 并截断后面
        //abcd  =>  b
        if (byte != (0xff)) {
            (*key)[i] = byte + 1;
            sdsResize(*key, i + 1, 1);
            return;
        }
    }
}
Comparator bytewiseComparator = {
    .getName = bytewiseComparatorName,
    .compare = bytewiseComparatorCompare,
    .findShortestSeparator = bytewiseComparatorFindShortestSeparator,
    .findShortSuccessor = bytewiseComparatorFindShortSuccessor
};