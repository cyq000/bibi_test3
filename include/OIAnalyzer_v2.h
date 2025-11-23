#pragma once
#include <string>
#include <vector>
#include <cmath>

#include "OIAnalyzer.h"

//// ---------- 单条数据 ----------
//struct OIData {
//    std::string symbol;
//    double OI = 0;     // 持仓量
//    double OIV = 0;    // 持仓价值
//    double price = 0;  // 推算价格 = OIV / OI
//    long long ts = 0;  // 时间戳
//};
//
//// ---------- 分析结果 ----------
//struct AnalysisResult {
//    int score = 0;      // 总分
//    std::string status; // 文字描述
//};

class OIAnalyzerV2 {
public:
    // 主分析入口
    AnalysisResult analyze(const std::vector<OIData>& data);

    // JSON 解析
    std::vector<OIData> parseJson(const std::string& raw);

private:

    // --- 特征计算 ---
    double calcSlope(const std::vector<double>& v);
    double calcSmoothness(const std::vector<double>& v);
    double calcStability(const std::vector<double>& v);

    // --- 评分体系 ---
    int scoreAccumulationPattern(
        const std::vector<double>& OI,
        const std::vector<double>& price,
        const std::vector<double>& OIV
    );

    std::string classify(int score);
};
