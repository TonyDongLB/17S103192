#include "stdafx.h"
#include "CppUnitTest.h"
#include "../P2PPeer/PeerClient.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace P2PPeerTest
{		
	TEST_CLASS(UnitTest1)
	{
	public:
		FileManage *fm;
		UnitTest1(){
		
			fm = new  FileManage;
		}
		~UnitTest1(){

			delete fm;
		}
		TEST_METHOD(TestMethod1)
		{
			// TODO:  �ڴ�������Դ���
			//���Զ�ȡ�ļ�
		
			int Real = fm->readFile("E:/a.mp3");
			int Expect = 0;
			Assert::AreEqual(Expect, Real);
		}

		TEST_METHOD(TestMethod2)
		{
			// TODO:  �ڴ�������Դ���
			//���Ի�ȡ�ļ�hashֵ
			string Real = fm->getFileHash("E:/a.mp3");
			string Expect = "98284D42919D469D63C3DF5F2CD99BEF4B92F632";
			Assert::AreEqual(Expect, Real);
		}
	};
}