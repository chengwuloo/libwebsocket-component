#ifndef STD_RANDOM_H
#define STD_RANDOM_H

#include <stdlib.h>
#include <stdio.h>
#include <memory>
#include <string.h>
#include <assert.h>
#include <algorithm>
#include <utility>
#include <random>
#include <vector>
#include <chrono>
//{std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())}
//{std::chrono::system_clock::now().time_since_epoch().count()}

//#define DEBUG_PRINT

//标准库的伪随机函数
namespace STD {
	//测试用例
	static void test001();
	static void test002(char const* filename);

	//////////////////////////////////////
	//Generator
	class Generator {
	public:
		Generator()
			: mt_({ std::random_device{}() })
			, re_({ std::random_device{}() }) {
		}
		static Generator& instance() {
			static Generator gen_;
			return gen_;
		}
		std::mt19937& get_mt() {
			return mt_;
		}
		std::default_random_engine& get_re() {
			return re_;
		}
	private:
		std::mt19937 mt_;
		std::default_random_engine re_;
	};

	//////////////////////////////////////
	//Random
	using RangeInt = std::uniform_int_distribution<>::param_type;
	using RangeFloat = std::uniform_real_distribution<float>::param_type;
	class Random {
	public:
		explicit Random() {}
		//整数范围
		explicit Random(int a, int b)
			: iValue_(a, b) {}
		//浮点范围
		explicit Random(float a, float b)
			: fValue_(a, b) {}
		//整数范围
		Random& betweenInt(int a, int b) {
			iValue_.param(RangeInt{ a, b });
			return *this;
		}
		//浮点范围
		Random& betweenFloat(float a, float b) {
			fValue_.param(RangeFloat{ a, b });
			return *this;
		}
	public:
		//mt生成器随机整数
		int randInt_mt(bool bv = false) {
			return iValue_(bv ? STD::Generator::instance().get_mt() : inst_.get_mt());
		}
		//re生成器随机整数
		int randInt_re(bool bv = false) {
			return iValue_(bv ? STD::Generator::instance().get_re() : inst_.get_re());
		}
		//mt生成器随机浮点数
		float randFloat_mt(bool bv = false) {
			return fValue_(bv ? STD::Generator::instance().get_mt() : inst_.get_mt());
		}
		//re生成器随机浮点数
		float randFloat_re(bool bv = false) {
			return fValue_(bv ? STD::Generator::instance().get_re() : inst_.get_re());
		}
	private:
		STD::Generator inst_;
		std::uniform_int_distribution<> iValue_;
		std::uniform_real_distribution<float> fValue_;
	};

	//////////////////////////////////////
	//Weight
	class Weight {
	public:
		//权重池
		Weight() {
			sum_ = 0;
		}
		//初始化权重集合
		void init(int weight[], int len)
		{
			indxId_.resize(len);
			weight_.resize(len);
			sum_ = 0;
			for (int i = 0; i < weight_.size(); ++i) {
				weight_[i] = weight[i];
				indxId_[i] = i;
				sum_ += weight[i];
			}
			if (sum_ <= 1) {
				return;
			}
			//随机数范围
			rand_.betweenInt(1, sum_);
		}
		//权重随机重排
		void shuffle() {
			for (int i = weight_.size() - 1; i > 0; --i) {
				std::uniform_int_distribution<decltype(i)> d(0, i);
				int j = d(STD::Generator::instance().get_mt());
				std::swap(weight_[i], weight_[j]);
				std::swap(indxId_[i], indxId_[j]);
			}
		}
		//按权值来随机，返回索引
		int getResult(bool bv = false) {
			if (sum_ <= 1) {
				return indxId_[0];
			}
#ifdef DEBUG_PRINT
			for (int i = 0; i < weight_.size(); ++i) {
				printf("w[%d]=%d\n", indxId_[i], weight_[i]);
			}
#endif
			int r = rand_.randInt_mt(bv), c = r;
			for (int i = 0; i < weight_.size(); ++i) {
				c -= weight_[i];
				if (c <= 0) {
#ifdef DEBUG_PRINT
					printf("sum=%d r=%d i=%d\n", sum_, r, indxId_[i]);
					printf("-------------------------\n\n\n");
#endif
					return indxId_[i];
				}
			}
		}
	public:
		STD::Random rand_;		 //随机数值
		int sum_;				 //权值之和
		std::vector<int> weight_;//权重集合
		std::vector<int> indxId_;//对应索引
	};

	//测试用例
	static void test001() {

		//随机数[5,10]
		STD::Random r1(5, 10);
		r1.randInt_mt();

		//随机数[5,10]
		STD::Random r2;
		r2.betweenInt(5, 10).randInt_mt();

		//随机浮点数[0.1,0.9]
		STD::Random r3(0.1f, 0.9f);
		r3.randFloat_mt();

		//随机浮点数[0.1,0.9]
		STD::Random r4;
		r4.betweenFloat(0.1f, 0.9f).randFloat_mt();

		//概率分别为30, 20, 50
		int weight[3] = { 30,20,50 };
		STD::Weight w;
		//初始化
		w.init(weight, 3);
		//随机10次
		for (int i = 0; i < 10; ++i) {
			//权值随机重排，可以不调用
			w.shuffle();
			//返回随机索引
			w.getResult();
		}
	}

	//测试按权重随机概率结果
	//写入文件再导入Excel并插入图表查看概率正态分布情况
	//filename char const* 要写入的文件 如/home/testweight.txt
	static void test002(char const* filename) {
		while (1) {
			if ('q' == getchar()) {
				break;
			}
			remove(filename);
			FILE* fp = fopen(filename, "a");
			if (fp == NULL) {
				return;
			}
			int c = 1000;			//循环总次数
			int scale = 10;			//放大倍数
			int ratioExC = 25;		//换牌概率
			int exC = 0, noExC = 0; //换牌/不换牌分别统计次数
			//概率分别为ratioExC, 100 - ratioExC
			int weight[2] = { ratioExC*scale,(100 - ratioExC)*scale };
			STD::Weight w;
			//初始化
			w.init(weight, 2);
			//随机c次
			for (int i = 0; i < c; ++i) {
				//权值随机重排，可以不调用
				w.shuffle();
				//返回随机索引
				int index = w.getResult();
				if (index == 0) {
					++exC;
				}
				else if (index == 1) {
					++noExC;
				}
				//写入文件再导入Excel并插入图表查看概率正态分布情况
				char ch[10] = { 0 };
				sprintf(ch, "%d\t", index == 0 ? 1 : -1);
				fwrite(ch, strlen(ch), 1, fp);
			}
			fflush(fp);
			fclose(fp);
			printf("c:%d:%d scale:%d ratioExC:%d exC:%d:ratio:%.02f\n",
				c, exC + noExC, scale, ratioExC,
				exC, ((float)exC) / (float)(exC + noExC));
		}
	}
}

#endif