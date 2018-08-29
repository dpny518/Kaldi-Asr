
#include "compute-center.h"
#include <boost/python.hpp>

using namespace boost::python;
using namespace compute;

BOOST_PYTHON_MODULE(libcompute)
{
	class_<ComputeCenter>("ComputeCenter")
	.def("ComputeScore", &ComputeCenter::ComputeScore)
	.def("TestPrint", &ComputeCenter::TestPrint)
	.def("ComputeSpeechScore", &ComputeCenter::ComputeSpeechScore)
	.def("GetSpeechJsonStrResult", &ComputeCenter::GetSpeechJsonStrResult)
	.def("GetAppJsonStrResult", &ComputeCenter::GetAppJsonStrResult)
	;
}
