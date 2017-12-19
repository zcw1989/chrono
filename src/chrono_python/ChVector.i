%{
#include <cstddef>
/* Includes the header in the wrapper code */
#include "chrono/core/ChVector.h"

%}


// HACK to deal with SWIG changing references to pointers
%extend chrono::ChVector<double> {
	double x() {return self->x();}
	double y() {return self->y();}
	double z() {return self->z();}
};

%ignore chrono::ChVector<double>::x();
%ignore chrono::ChVector<double>::y();
%ignore chrono::ChVector<double>::z();


/* Parse the header file to generate wrappers */
%include "../chrono/core/ChVector.h"  


%template(ChVectorD) chrono::ChVector<double>; 
%template(ChVectorF) chrono::ChVector<float>; 

// This is needed because a std::vector<ChVector<double>
// might be used  somewhere, and we want to use it via python:
%template(vector_ChVectorD) std::vector< chrono::ChVector<double> >;

// Constants seem not to work...
// %constant chrono::ChVector<double> VNULL = chrono::ChVector<double>(0,0,0);
// %constant chrono::ChVector<double> VECT_X= chrono::ChVector<double>(1,0,0);
// %constant chrono::ChVector<double> VECT_Y= chrono::ChVector<double>(0,1,0);
// %constant chrono::ChVector<double> VECT_Z= chrono::ChVector<double>(0,0,1);


%extend chrono::ChVector<double>{
		public:
					// Add function to support python 'print(...)'
			char *__str__() 
					{
						static char temp[256];
						sprintf(temp,"[ %g, %g, %g ]", $self->x(),$self->y(),$self->z());
						return &temp[0];
					}
					// operator  ^  as ^ in c++ 
			double __xor__(const ChVector<double>& other) const 
					{ 
						return $self->operator^(other);
					}
		};


// This because constants do not work well, so implement them in script-side

%pythoncode %{

	VNULL  = ChVectorD(0,0,0)
	VECT_X = ChVectorD(1,0,0)
	VECT_Y = ChVectorD(0,1,0)
	VECT_Z = ChVectorD(0,0,1)

%}