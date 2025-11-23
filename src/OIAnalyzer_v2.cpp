#include "OIAnalyzer_v2.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

/*
=================================================
                JSON 解析
=================================================
*/
std::vector<OIData> OIAnalyzerV2::parseJson(const std::string& raw) {
    auto j = json::parse(raw);
    std::vector<OIData> out;

    for (auto& e : j) {
        OIData d;
        d.symbol = e["symbol"].get<std::string>();
        d.OI = std::stod(e["sumOpenInterest"].get<std::string>());
        d.OIV = std::stod(e["sumOpenInterestValue"].get<std::string>());
        d.price = (d.OI > 0 ? d.OIV / d.OI : 0);
        d.ts = e["timestamp"].get<long long>();
        out.push_back(d);
    }
    return out;
}

/*
=================================================
          工具函数：线性回归斜率
  用于判断整体趋势（上涨、下跌、平缓）
=================================================
*/
double OIAnalyzerV2::calcSlope(const std::vector<double>& v) {
    int n = v.size();
    if (n <= 1) return 0;

    double sumX = 0, sumY = 0, sumXY = 0, sumXX = 0;
    for (int i = 0; i < n; i++) {
        sumX += i;
        sumY += v[i];
        sumXY += i * v[i];
        sumXX += i * i;
    }
    double denom = (n * sumXX - sumX * sumX);
    if (denom == 0) return 0;

    return (n * sumXY - sumX * sumY) / denom;
}

/*
=================================================
          曲线平滑度：检测是否有大波动噪音
  数值越低越平滑，越高越波动
=================================================
*/
double OIAnalyzerV2::calcSmoothness(const std::vector<double>& v) {
    if (v.size() < 2) return 0;
    double sum = 0;
    for (size_t i = 1; i < v.size(); i++)
        sum += std::abs(v[i] - v[i - 1]);
    return sum / v.size();
}

/*
=================================================
              稳定度评分（越稳定越高）
=================================================
*/
double OIAnalyzerV2::calcStability(const std::vector<double>& v) {
    if (v.size() < 2) return 1;
    double mean = 0;
    for (auto x : v) mean += x;
    mean /= v.size();

    double var = 0;
    for (auto x : v) var += (x - mean) * (x - mean);
    var /= v.size();
    return 1.0 / (1.0 + std::sqrt(var));  // 越稳定越接近 1
}

/*
=================================================
            V2 核心结构评分体系
  --- 识别你想要的“潜在吸筹结构”
=================================================

评分逻辑：
1. OI 斜率略微向下 或 基本平 → 属于“低吸筹结构”
2. price 稳定下降 → 庄家压价格
3. OIV 稳定 → 不出现暴涨暴跌
4. 平滑度高（波动小）→ 低噪音走势
*/
int OIAnalyzerV2::scoreAccumulationPattern(
    const std::vector<double>& OI,
    const std::vector<double>& price,
    const std::vector<double>& OIV
) {
    int score = 0;

    // --- 计算基本特征 ---
    double slope_OI = calcSlope(OI);
    double slope_price = calcSlope(price);
    double sm_OI = calcSmoothness(OI);
    double sm_price = calcSmoothness(price);
    double st_price = calcStability(price);
    double st_OIV = calcStability(OIV);

    // --- 评分规则 ---

    // A) 持仓量 OI：缓慢下降 or 平稳 → 可能在换手吸筹
    if (slope_OI < 0 && slope_OI > -0.005) score += 20;
    if (std::abs(slope_OI) < 0.0015) score += 15; // 很平 →

    // B) 价格：缓慢下跌 → 庄方压价
    if (slope_price < 0) score += 20;
    if (slope_price < 0 && std::abs(slope_price) < 0.01) score += 10;

    // C) 价格波动平滑
    if (sm_price < 0.015) score += 15;

    // D) OIV 稳定 → 资金持续未撤离
    score += (int)(st_OIV * 20);

    // E) 小波动模式 → 更像吸筹
    if (sm_OI < 0.02) score += 10;

    return score;
}

/*
=================================================
                主分析入口
=================================================
*/
AnalysisResult OIAnalyzerV2::analyze(const std::vector<OIData>& data)
{
    AnalysisResult R;
    if (data.size() < 10) {
        R.status = "数据不足";
        return R;
    }

    std::vector<double> OI, price, OIV;
    for (auto& d : data) {
        OI.push_back(d.OI);
        price.push_back(d.price);
        OIV.push_back(d.OIV);
    }

    R.score = scoreAccumulationPattern(OI, price, OIV);
    R.status = classify(R.score);
    return R;
}

/*
=================================================
          状态分类：你可继续调节阈值
=================================================
*/
std::string OIAnalyzerV2::classify(int score) {
    if (score >= 80) return "强吸筹结构（重点监控）";
    if (score >= 60) return "弱吸筹结构（可关注）";
    if (score >= 40) return "一般走势（不推荐）";
    return "无吸筹结构";
}
