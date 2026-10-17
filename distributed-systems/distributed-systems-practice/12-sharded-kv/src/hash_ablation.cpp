// hash_ablation — does the fmix64 finalizer in hash_ring.h actually matter? Builds the same ring with plain FNV-1a and with FNV-1a+fmix64 and compares balance (20 salted rings x 100000 keys). Self-contained copy of the two hash functions.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
static uint64_t fmix64(uint64_t k){k^=k>>33;k*=0xff51afd7ed558ccdULL;k^=k>>33;k*=0xc4ceb9fe1a85ec53ULL;k^=k>>33;return k;}
static uint64_t fnv(const std::string&s){uint64_t h=1469598103934665603ULL;for(unsigned char c:s){h^=c;h*=1099511628211ULL;}return h;}
static uint64_t mixed(const std::string&s){return fmix64(fnv(s));}
template<class H> void run(const char* name,H h,int shards,int vnodes){
  const int salts=20, keys=100000; double cvsum=0, maxsum=0;
  for(int s=0;s<salts;s++){
    std::vector<std::pair<uint64_t,int>> pts;
    for(int id=1;id<=shards;id++)for(int v=0;v<vnodes;v++)pts.emplace_back(h(std::to_string(s)+"shard-"+std::to_string(id)+"#"+std::to_string(v)),id);
    std::sort(pts.begin(),pts.end());
    std::vector<double> load(shards+1,0);
    for(int k=0;k<keys;k++){uint64_t x=h("key-"+std::to_string(k));auto it=std::lower_bound(pts.begin(),pts.end(),std::make_pair(x,0));if(it==pts.end())it=pts.begin();load[it->second]++;}
    double mean=(double)keys/shards,var=0,mx=0;for(int i=1;i<=shards;i++){var+=(load[i]-mean)*(load[i]-mean);mx=std::max(mx,load[i]);}
    cvsum+=std::sqrt(var/shards)/mean; maxsum+=mx/mean;
  }
  std::printf("  %-22s shards=%d vnodes=%-4d  mean cv=%.4f  mean max/mean=%.3f\n",name,shards,vnodes,cvsum/salts,maxsum/salts);
}
int main(){
  for(int shards:{3,10})for(int vn:{16,128}){run("FNV-1a only",fnv,shards,vn);run("FNV-1a + fmix64",mixed,shards,vn);}
}
