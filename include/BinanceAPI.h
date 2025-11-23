#ifndef BINANCE_API_H
#define BINANCE_API_H

#include <string>
#include <vector>


// struct BianOIData {
//     std::string symbol;
//     double OI;
//     double OIV;
//     double price;
//     long long ts;
// };

class BinanceAPI {
public:
    BinanceAPI();

    /* ============公共方法============ */
    /*
    * 获取 24 小时成交额在区间内的所有合约币种
    * 只监控 USDT 结算的交易对
    * long minVol 最小值
    * long maxVol 最大值
    * 返回符合的币种数组
    */
    std::vector<std::string> getSymbolsWithVolume(long minVol, long maxVol);

    /*
    * 或者指定币种的持仓量数据
    * const std::string& symbol 币种名称
    * const std::string& period 时间周期
    * int limit K线数
    */
    //std::vector<BianOIData> fetchOpenInterestData(const std::string& symbol, const std::string& period, int limit);
    std::string fetchOpenInterestData(const std::string& symbol, const std::string& period, int limit);
    
private:
    std::string httpGet(const std::string& url);
};

#endif
