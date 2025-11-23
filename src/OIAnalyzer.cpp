#include "OIAnalyzer.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

double OIAnalyzer::changeRate(double curr, double prev) {
    if (prev == 0) return 0.0;
    return (curr - prev) / prev;
}

int OIAnalyzer::direction(double x) {
    if (x > 0) return 1;
    if (x < 0) return -1;
    return 0;
}


/* 
    目标是对一条时间序列（OI / OIV / 推算价格）打分，判断该币种是否出现“OI↑ 且 price↓ 的吸筹 / 建仓结构”。
    评分由单周期因子、连续性因子、二阶导（加速度）等组合而成，最后根据总分分类（classify）。
*/
 //AnalysisResult OIAnalyzer::analyze(const std::vector<OIData>& data) {
 //    AnalysisResult result;
 //    int n = data.size();
 //    if (n < 20) return result;  // 无意义的情形。

 //    int& score = result.score;
 //    int& OI_up_count = result.OI_up_count;
 //    int OI_up_count_last = 0;
 //    int& Price_down_count = result.Price_down_count;
 //    int Price_down_count_last = 0;

 //    for (int i = 1; i < n; i++) {
 //        const OIData& prev = data[i - 1];
 //        const OIData& curr = data[i];

 //        // 计算三项相对变化率（百分比）
 //        double dOI    = changeRate(curr.OI, prev.OI);
 //        double dOIV   = changeRate(curr.OIV, prev.OIV);
 //        double dPrice = changeRate(curr.price, prev.price);

 //        /*   
 //        ------------------------
 //        1、OI 因子（资金入场信号）
 //        目的：给持仓量上升的周期加分。
 //        两档加分：
 //        只要 dOI > 0 加 1 分（表示方向为“上”）
 //        如果 dOI > 5%（单周期增幅>5%）再加 1 分（表示强烈流入）
 //        同时维护连续上升计数 OI_up_count（若本周期上升则计数加1，否则归 0）。
 //        ------------------------
 //        */
 //        if (dOI > 0) score += 1;
 //        if (dOI > 0.05) score += 1;
 //        if (dOI > 0){
 //            OI_up_count++;
 //            OI_up_count_last = OI_up_count;
 //        }else {
 //            OI_up_count = 0;
 //        }

 //        /*
 //        ------------------------
 //        2、价格因子（价格走弱信号）
 //        目的：检测价格下行（或无涨）的周期并加分（价跌是“吸筹”重要条件）。
 //        两档加分：
 //        价格下降（dPrice < 0）加 1
 //        价格下降超过 3%（dPrice < -3%）再加 1
 //        Price_down_count 记录连续下跌周期数（用于后续连续性加分）。
 //        ------------------------
 //        */
 //        if (dPrice < 0) score += 1;
 //        if (dPrice < -0.03) score += 1;
 //        if (dPrice < 0){
 //            Price_down_count++;
 //            Price_down_count_last = Price_down_count;
 //        }else {
 //            Price_down_count = 0;
 //        }

 //        /* 
 //        -------------------------
 //        3、OIV（持仓价值）与 OI 的对比 — 吸筹特征
 //        上涨弱于 OI → 吸筹结构
 //        目的：如果 OI 在上涨但 OIV（名义价值）上涨 明显弱于 OI 的上涨（这里只用 <= 30% 的判断），说明以更低价格吃入更多合约——这是吸筹的重要信号。
 //        逻辑解释：
 //        OI↑ 表示开仓/持仓增加
 //        但 OIV（≈ OI × avg_price）并未同比例上升，意味着平均价格下降或OIV几乎不变→更多合约在更低价格被持有。
 //        ------------------------- 
 //        */
 //        if (dOI > 0 && dOIV <= dOI * 0.3)
 //            score += 2;

 //        /*
 //        -------------------------
 //        4、方向差异（OI 与 Price 反向）
 //        如果 OI 上（+1）而 Price 下（-1），额外加 1 分。
 //        这对“OI↑且Price↓”这一核心结构再做一次确认，加分较小但稳定。
 //        注意：这里与3重复部分验证，但更宽松（只看方向，不看幅度）。
 //        -------------------------
 //        */
 //        int dirOI = direction(dOI);
 //        int dirPrice = direction(dPrice);
 //        if (dirOI == 1 && dirPrice == -1)
 //            score += 1;

 //        /*
 //        -------------------------
 //        5、加速度（第二阶导，资金流入是否加速）
 //        计算当前 dOI 与上一个周期 dOI 的差（近似二阶导）。
 //        如果 dOI 在加速（即资金流入速度变快），加 1 分。
 //        理由：加速通常预示着更强的主导力量进入（更接近要爆发的阶段）。
 //        注意：
 //        这对捕捉爆发性拉升或大规模建仓非常有用。
 //        但受噪声影响大，建议配合平滑或限制最大增幅（用 rolling mean）。
 //        -------------------------
 //        */
 //        if (i >= 2) {
 //            double prev_dOI = changeRate(prev.OI, data[i - 2].OI);
 //            if ((dOI - prev_dOI) > 0)
 //                score += 1;
 //        }
 //    }

 //    /* 
 //    -------------------------
 //    连续性加分（补强长期模式）
 //    如果某方向连续出现 ≥ 3 个周期，再给额外分数（每项 +2）。
 //    这把单周期的信号扩展为“连续结构”判断：更稳健、更可信。
 //    -------------------------
 //    */
 //    if (OI_up_count_last >= 3)    score += 2;
 //    if (Price_down_count_last >= 3) score += 2;

 //    /* 最终分类 */
 //    result.status = classify(result);

 //    return result;
 //}

std::string OIAnalyzer::classify(const AnalysisResult& r) {
    if (r.score >= 80) return "Strong Accumulation";
    if (r.score >= 60) return "Possible Accumulation";
    if (r.score >= 40) return "Neutral / No Clear Signal";
    return "No Accumulation";
}


// ===============================
/* 解析 JSON → vector<OIData> */
// ===============================
std::vector<OIData> OIAnalyzer::parseJson(const std::string& raw) {
    std::vector<OIData> out;

    json j = json::parse(raw);

    for (auto& item : j) {
        OIData d;
        d.symbol = item["symbol"];
        d.OI = stod(item["sumOpenInterest"].get<std::string>());
        d.OIV = stod(item["sumOpenInterestValue"].get<std::string>());
        d.ts = item["timestamp"];

        /* 推算价格 */
        d.price = (d.OIV / d.OI);

        out.push_back(d);
    }

    return out;
}


/* 越平滑、越稳步上涨，得分越高（0–1）*/
double OIAnalyzer::smoothScore(const std::vector<double>& v) {
    if (v.size() < 3) return 0;

    double score = 1.0;
    for (size_t i = 2; i < v.size(); i++) {
        double a = v[i] - v[i - 1];
        double b = v[i - 1] - v[i - 2];

        /* 一致趋势 → 加分 */
        if (a * b > 0 && a > 0)
            score += 0.3;

        /* 波动太大 → 扣分 */
        if (std::fabs(a - b) > (0.08 * v[i]))
            score -= 0.2;
    }

    if (score < 0) score = 0;
    if (score > 5) score = 5;

    return score / 5.0; // 归一化
}

/* ============= 核心分析逻辑（全新评分体系） ============= */

AnalysisResult OIAnalyzer::analyze(const std::vector<OIData>& data) {
    AnalysisResult R;

    if (data.size() < 3)
        return R;

    /* ---------- 收集序列 ---------- */
    std::vector<double> oi, oiv, price;
    for (auto& d : data) {
        oi.push_back(d.OI);
        oiv.push_back(d.OIV);
        price.push_back(d.price);
    }

    /* ---------- 1. OI 稳定增长得分（60 分） ---------- */
    double sOI = smoothScore(oi);
    int OI_score = (int)(sOI * 60);

    /* ---------- 2. OIV 波动特征（20 分） ---------- */
    double sOIV = smoothScore(oiv);
    int OIV_score = (int)(sOIV * 20);

    /* ---------- 3. Price 横盘分（20 分） ---------- */
    double priceVol = 0;
    for (size_t i = 1; i < price.size(); i++) {
        priceVol += std::fabs(changeRate(price[i], price[i - 1]));
    }
    priceVol /= (price.size() - 1);

    int Price_score = 0;
    if (priceVol < 0.01)
        Price_score = 20; // 极度横盘 → 最高分
    else if (priceVol < 0.02)
        Price_score = 15;
    else if (priceVol < 0.05)
        Price_score = 8;

    /* ----------- 汇总总分 ----------- */
    R.score = OI_score + OIV_score + Price_score;

    /* ----------- 辅助计数（非评分用）----------- */
    for (size_t i = 1; i < data.size(); i++) {
        if (oi[i] > oi[i - 1]) R.OI_up_count++;
        if (price[i] < price[i - 1]) R.Price_down_count++;
    }

    /* ----------- 状态分类 ----------- */
    R.status = classify(R);
    return R;
}