//
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2010-2012 Alessandro Tasora
// Copyright (c) 2013 Project Chrono
// All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file at the top level of the distribution
// and at http://projectchrono.org/license-chrono.txt.
//

#ifndef CHARCHIVE_H
#define CHARCHIVE_H


#include "core/ChApiCE.h"
#include "core/ChStream.h"
#include "core/ChSmartpointers.h"
#include "core/ChTemplateExpressions.h"
#include <string>
#include <vector>
#include <typeinfo>

namespace chrono {

// forward reference
class ChArchiveOut;
class ChArchiveIn;


/// Macro to create a  ChDetect_GetRTTI  detector that can be used in 
/// templates, to select which specialized template to use depending if the
/// GetRTTI function is present in the class of type T.
CH_CREATE_MEMBER_DETECTOR(GetRTTI)




/// Functor to call the ArchiveOUT function for unrelated classes that
/// implemented them. This helps stripping out the templating, to make ChArchiveOut
/// easier and equippable with virtual functions.

class ChFunctorArchiveOut {
public:
    virtual void CallArchiveOut(ChArchiveOut& marchive)=0;    
};

template <class TClass> 
class ChFunctorArchiveOutSpecific : public ChFunctorArchiveOut
{
private:
      void (TClass::*fpt)(ChArchiveOut&);   // pointer to member function
      TClass* pt2Object;                    // pointer to object

public:

      // constructor - takes pointer to an object and pointer to a member
      ChFunctorArchiveOutSpecific(TClass* _pt2Object, void(TClass::*_fpt)(ChArchiveOut&))
         { pt2Object = _pt2Object;  fpt=_fpt; };

      virtual void CallArchiveOut(ChArchiveOut& marchive)
        { (*pt2Object.*fpt)(marchive);};             // execute member function
};


/// Functor to call the ArchiveIN function for unrelated classes that
/// implemented them. This helps stripping out the templating, to make ChArchiveOut
/// easier and equippable with virtual functions.

class ChFunctorArchiveIn {
public:
    virtual void CallArchiveIn(ChArchiveIn& marchive)=0;
    virtual void CallNew(ChArchiveIn& marchive) {};
    virtual void CallNewAbstract(ChArchiveIn& marchive, const char* classname) {};
    virtual void  CallSetRawPtr(ChArchiveIn& marchive, void* mptr) {};
    virtual void* CallGetRawPtr(ChArchiveIn& marchive) { return 0;};
};

template <class TClass> 
class ChFunctorArchiveInSpecific : public ChFunctorArchiveIn
{
private:
      void (TClass::*fpt)(ChArchiveIn&);   // pointer to member function
      TClass* pt2Object;                    // pointer to object

public:

      // constructor - takes pointer to an object and pointer to a member 
      ChFunctorArchiveInSpecific(TClass* _pt2Object, void(TClass::*_fpt)(ChArchiveIn&))
         { pt2Object = _pt2Object;  fpt=_fpt; };

      virtual void CallArchiveIn(ChArchiveIn& marchive)
        { (*pt2Object.*fpt)(marchive);};             // execute member function
};

template <class TClass> 
class ChFunctorArchiveInSpecificPtr : public ChFunctorArchiveIn
{
private:
      void (TClass::*fpt)(ChArchiveIn&);   // pointer to member function
      TClass** pt2Object;                    // pointer to object

public:

      // constructor - takes pointer to an object and pointer to a member 
      ChFunctorArchiveInSpecificPtr(TClass** _pt2Object, void(TClass::*_fpt)(ChArchiveIn&))
         { pt2Object = _pt2Object;  fpt=_fpt; };

      virtual void CallArchiveIn(ChArchiveIn& marchive)
        { (**pt2Object.*fpt)(marchive);};             // execute member function

      virtual void CallNew(ChArchiveIn& marchive)
        { *pt2Object = new(TClass); }

      virtual void  CallSetRawPtr(ChArchiveIn& marchive, void* mptr) 
        { *pt2Object = static_cast<TClass*>(mptr); };

      virtual void* CallGetRawPtr(ChArchiveIn& marchive) 
        { return static_cast<void*>(*pt2Object); };
};


template <class TClass> 
class ChFunctorArchiveInSpecificPtrAbstract : public ChFunctorArchiveIn
{
private:
      void (TClass::*fpt)(ChArchiveIn&);   // pointer to member function
      TClass** pt2Object;                    // pointer to object

public:

      // constructor - takes pointer to an object and pointer to a member 
      ChFunctorArchiveInSpecificPtrAbstract(TClass** _pt2Object, void(TClass::*_fpt)(ChArchiveIn&))
        { pt2Object = _pt2Object;  fpt=_fpt; };

      virtual void CallArchiveIn(ChArchiveIn& marchive)
        { (**pt2Object.*fpt)(marchive);};             // execute member function

      virtual void CallNewAbstract(ChArchiveIn& marchive, const char* classname)
        { chrono::create(classname, pt2Object); }

      virtual void  CallSetRawPtr(ChArchiveIn& marchive, void* mptr) 
        { *pt2Object = static_cast<TClass*>(mptr); };

      virtual void* CallGetRawPtr(ChArchiveIn& marchive) 
        { return static_cast<void*>(*pt2Object); };
};




///
/// This is a base class for name-value pairs
///

template<class T>
class  ChNameValue {
  public:
        ChNameValue(const char* mname, T& mvalue) : _name(mname), _value((T*)(& mvalue)) {}

        virtual ~ChNameValue() {};

        const char * name() const {
            return this->_name;
        }

        T & value() const {
            return *(this->_value);
        }

        const T & const_value() const {
            return *(this->_value);
        }

  protected:
        T* _value;
        const char* _name;
};



template<class T>
ChNameValue< T > make_ChNameValue(const char * name, T & t){
    return ChNameValue< T >(name, t);
}

/// Macros to create ChNameValue objects easier

#define CHNVP2(name,val) \
    make_ChNameValue(name,val)

#define CHNVP(val) \
    make_ChNameValue(#val,val)



///
/// This is a base class for archives with pointers to shared objects 
///

class ChArchive {
  protected:

    /// vector of pointers to stored/retrieved objects,
    /// to avoid saving duplicates or deadlocks
    std::vector<void*> objects_pointers;

    bool use_versions;

  public:
    ChArchive() {
        use_versions = true;
        Init();
    }

    virtual ~ChArchive() {};

    /// Reinitialize the vector of pointers to loaded/saved objects
    void Init() {
        objects_pointers.clear();
        objects_pointers.push_back(0);
    }
    /// Put a pointer in pointer vector, but only if it
    /// was not previously inserted. Returns position of pointer
    /// if already existing, otherwise -1.
    void PutPointer(void* object, bool& already_stored, size_t& pos) {
        for (size_t i = 0; i < objects_pointers.size(); ++i) {
            if (objects_pointers[i] == object)
            {
                already_stored = true;
                pos = i;
                return;
            }
        }
        // wasn't in list.. add to it
        objects_pointers.push_back(object);

        already_stored = false;
        pos = objects_pointers.size()-1;
        return;
    }

    /// By default, version numbers are saved in archives
    /// Use this to turn off version info in archives (either save/load both
    /// with version info, or not, do not mix because it could give problems in binary archives.).
    void SetUseVersions(bool muse) {this->use_versions = muse;}
};



///
/// This is a base class for serializing into archives
///

class  ChArchiveOut : public ChArchive {
  public:
      virtual ~ChArchiveOut() {};

      //---------------------------------------------------
      // INTERFACES - to be implemented by children classes
      //

        // for integral types:
      virtual void out     (ChNameValue<bool> bVal) = 0;
      virtual void out     (ChNameValue<int> bVal) = 0;
      virtual void out     (ChNameValue<double> bVal) = 0;
      virtual void out     (ChNameValue<float> bVal) = 0;
      virtual void out     (ChNameValue<char> bVal) = 0;
      virtual void out     (ChNameValue<unsigned int> bVal) =0;
      virtual void out     (ChNameValue<std::string> bVal) = 0;
      virtual void out     (ChNameValue<unsigned long> bVal) = 0;
      virtual void out     (ChNameValue<unsigned long long> bVal) =0;

        // for custom C++ objects - see 'wrapping' trick below
      virtual void out     (ChNameValue<ChFunctorArchiveOut> bVal, const char* classname) = 0;

        // for pointed objects with abstract class system (i.e. supporting class factory)
      virtual void out_ref_abstract (ChNameValue<ChFunctorArchiveOut> bVal, bool already_inserted, size_t position, const char* classname) = 0;
      
        // for pointed objects without class abstraction
      virtual void out_ref          (ChNameValue<ChFunctorArchiveOut> bVal, bool already_inserted, size_t position, const char* classname) = 0;

        // for wrapping arrays and lists
      virtual void out_array_pre (const char* name, size_t msize, const char* classname) = 0;
      virtual void out_array_between (size_t msize, const char* classname) = 0;
      virtual void out_array_end (size_t msize,const char* classname) = 0;


      //---------------------------------------------------

        // trick to wrap C++ fixed-size arrays:
      template<class T, size_t N>
      void out     (ChNameValue<T[N]> bVal) {
          size_t arraysize = sizeof(bVal.value())/sizeof(T);
          this->out_array_pre(bVal.name(), arraysize, typeid(T).name());
          for (size_t i = 0; i<arraysize; ++i)
          {
              char buffer[20];
              sprintf(buffer, "el_%d", i);
              ChNameValue< T > array_val(buffer, bVal.value()[i]);
              this->out (array_val);
              this->out_array_between(arraysize, typeid(bVal.value()).name());
          }
          this->out_array_end(arraysize, typeid(bVal.value()).name());
      }

        // trick to wrap stl::vector container
      template<class T>
      void out     (ChNameValue< std::vector<T> > bVal) {
          this->out_array_pre(bVal.name(), bVal.value().size(), typeid(T).name());
          for (size_t i = 0; i<bVal.value().size(); ++i)
          {
              char buffer[20];
              sprintf(buffer, "el_%d", i);
              ChNameValue< T > array_val(buffer, bVal.value()[i]);
              this->out (array_val);
              this->out_array_between(bVal.value().size(), typeid(bVal.value()).name());
          }
          this->out_array_end(bVal.value().size(), typeid(bVal.value()).name());
      }

        // trick to call out_ref on ChSharedPointer, with class abstraction:
      template<class T, typename enable_if< ChDetect_GetRTTI<T>::value, T>::type* = nullptr > 
      void out     (ChNameValue< ChSharedPtr<T> > bVal) {
          bool already_stored; size_t pos;
          PutPointer(bVal.value().get_ptr(), already_stored, pos);
          ChFunctorArchiveOutSpecific<T> specFuncA(bVal.value().get_ptr(), &T::ArchiveOUT);
          this->out_ref_abstract(
              ChNameValue<ChFunctorArchiveOut>(bVal.name(), specFuncA), 
              already_stored, 
              pos, 
              bVal.value()->GetRTTI()->GetName() );
      }

        // trick to call out_ref on ChSharedPointer, without class abstraction:
      template<class T, typename enable_if< !ChDetect_GetRTTI<T>::value, T>::type* = nullptr >
      void out     (ChNameValue< ChSharedPtr<T> > bVal) {
          bool already_stored; size_t pos;
          PutPointer(bVal.value().get_ptr(), already_stored, pos);
          ChFunctorArchiveOutSpecific<T> specFuncA(bVal.value().get_ptr(), &T::ArchiveOUT);
          this->out_ref(
              ChNameValue<ChFunctorArchiveOut>(bVal.name(), specFuncA), 
              already_stored, 
              pos, 
              typeid(T).name() ); // note, this class name is not platform independent
      }

        // trick to call out_ref on plain pointers, with class abstraction:
      template<class T,  typename enable_if< ChDetect_GetRTTI<T>::value, T>::type* = nullptr >
      void out     (ChNameValue<T*> bVal) {
          bool already_stored; size_t pos;
          PutPointer(bVal.value(), already_stored, pos);
          ChFunctorArchiveOutSpecific<T> specFuncA(bVal.value(), &T::ArchiveOUT);
          this->out_ref_abstract(
              ChNameValue<ChFunctorArchiveOut>(bVal.name(), specFuncA), 
              already_stored,
              pos, 
              bVal.value()->GetRTTI()->GetName() ); // this class name is platform independent
      }

         // trick to call out_ref on plain pointers (no class abstraction):
      template<class T, typename enable_if< !ChDetect_GetRTTI<T>::value, T>::type* = nullptr >
      void out     (ChNameValue<T*> bVal) {
          bool already_stored; size_t pos;
          PutPointer(bVal.value(), already_stored, pos);
          ChFunctorArchiveOutSpecific<T> specFuncA(bVal.value(), &T::ArchiveOUT);
          this->out_ref(
              ChNameValue<ChFunctorArchiveOut>(bVal.name(), specFuncA), 
              already_stored,
              pos, 
              typeid(T).name() ); // note, this class name is not platform independent
      }

        // trick to apply 'virtual out..' on unrelated C++ objects that has a function "ArchiveOUT":
      template<class T>
      void out     (ChNameValue<T> bVal) {
          ChFunctorArchiveOutSpecific<T> specFuncA(&bVal.value(), &T::ArchiveOUT);
          this->out(
              ChNameValue<ChFunctorArchiveOut>(bVal.name(), specFuncA), 
              typeid(T).name() );
      }

        /// Operator to allow easy serialization as   myarchive >> mydata;
      
      template<class T>
      ChArchiveOut& operator<<(ChNameValue<T> bVal) {
          this->out(bVal);
          return (*this);
      }
      
      void VersionWrite(int mver) {
          if (use_versions)
            this->out(ChNameValue<int>("version",mver));
      }
      
  protected:

};



///
/// This is a base class for serializing from archives
///

class  ChArchiveIn : public ChArchive {
  public:
      virtual ~ChArchiveIn() {};

      //---------------------------------------------------
      // INTERFACES - to be implemented by children classes
      //

        // for integral types:
      virtual void in     (ChNameValue<bool> bVal) = 0;
      virtual void in     (ChNameValue<int> bVal) = 0;
      virtual void in     (ChNameValue<double> bVal) = 0;
      virtual void in     (ChNameValue<float> bVal) = 0;
      virtual void in     (ChNameValue<char> bVal) = 0;
      virtual void in     (ChNameValue<unsigned int> bVal) = 0;
      virtual void in     (ChNameValue<std::string> bVal) = 0;
      virtual void in     (ChNameValue<unsigned long> bVal) = 0;
      virtual void in     (ChNameValue<unsigned long long> bVal) = 0;

        // for custom C++ objects - see 'wrapping' trick below
      virtual void in     (ChNameValue<ChFunctorArchiveIn> bVal) = 0;

        // for pointed objects 
      virtual void in_ref_abstract (ChNameValue<ChFunctorArchiveIn> bVal) = 0;
      virtual void in_ref          (ChNameValue<ChFunctorArchiveIn> bVal) = 0;

        // for wrapping arrays and lists
      virtual void in_array_pre (const char* name, size_t& msize) = 0;
      virtual void in_array_between (const char* name) = 0;
      virtual void in_array_end (const char* name) = 0;

      //---------------------------------------------------

             // trick to wrap C++ fixed-size arrays:
      template<class T, size_t N>
      void in     (ChNameValue<T[N]> bVal) {
          size_t arraysize;
          this->in_array_pre(bVal.name(), arraysize);
          if (arraysize != sizeof(bVal.value())/sizeof(T) ) {throw (ChException( "Size of [] saved array does not match size of receiver array " + std::string(bVal.name()) + "."));}
          for (size_t i = 0; i<arraysize; ++i)
          {
              char idname[20];
              sprintf(idname, "el_%d", i);
              T element;
              ChNameValue< T > array_val(idname, element);
              this->in (array_val);
              bVal.value()[i]=element;
              this->in_array_between(bVal.name());
          }
          this->in_array_end(bVal.name());
      }

              // trick to wrap stl::vector container
      template<class T>
      void in     (ChNameValue< std::vector<T> > bVal) {
          bVal.value().clear();
          size_t arraysize;
          this->in_array_pre(bVal.name(), arraysize);
          bVal.value().resize(arraysize);
          for (size_t i = 0; i<arraysize; ++i)
          {
              char idname[20];
              sprintf(idname, "el_%d", i);
              T element;
              ChNameValue< T > array_val(idname, element);
              this->in (array_val);
              bVal.value()[i]=element;
              this->in_array_between(bVal.name());
          }
          this->in_array_end(bVal.name());
      }

        // trick to call in_ref on ChSharedPointer, with class abstraction:
      template<class T, typename enable_if< ChDetect_GetRTTI<T>::value, T>::type* = nullptr >
      void in     (ChNameValue< ChSharedPtr<T> > bVal) {
          T* mptr;
          ChFunctorArchiveInSpecificPtrAbstract<T> specFuncA(&mptr, &T::ArchiveIN);
          ChNameValue<ChFunctorArchiveIn> mtmp(bVal.name(), specFuncA);
          this->in_ref_abstract(mtmp);
          bVal.value() = ChSharedPtr<T> ( mptr );
      }

         // trick to call in_ref on ChSharedPointer (no class abstraction):
      template<class T, typename enable_if< !ChDetect_GetRTTI<T>::value, T>::type* = nullptr >
      void in     (ChNameValue< ChSharedPtr<T> > bVal) {
          T* mptr;
          ChFunctorArchiveInSpecificPtr<T> specFuncA(&mptr, &T::ArchiveIN);
          ChNameValue<ChFunctorArchiveIn> mtmp(bVal.name(), specFuncA);
          this->in_ref(mtmp);
          bVal.value() = ChSharedPtr<T> ( mptr );
      }

        // trick to call in_ref on plain pointers, with class abstraction:
      template<class T, typename enable_if< ChDetect_GetRTTI<T>::value, T>::type* = nullptr >
      void in     (ChNameValue<T*> bVal) {
          ChFunctorArchiveInSpecificPtrAbstract<T> specFuncA(&bVal.value(), &T::ArchiveIN);
          this->in_ref_abstract(ChNameValue<ChFunctorArchiveIn>(bVal.name(), specFuncA) );
      }

        // trick to call in_ref on plain pointers (no class abstraction):
      template<class T, typename enable_if< !ChDetect_GetRTTI<T>::value, T>::type* = nullptr >
      void in     (ChNameValue<T*> bVal) {
          ChFunctorArchiveInSpecificPtr<T> specFuncA(&bVal.value(), &T::ArchiveIN);
          this->in_ref(ChNameValue<ChFunctorArchiveIn>(bVal.name(), specFuncA) );
      }

        // trick to apply 'virtual in..' on C++ objects that has a function "ArchiveIN":
      template<class T>
      void in     (ChNameValue<T> bVal) {
          ChFunctorArchiveInSpecific<T> specFuncA(&bVal.value(), &T::ArchiveIN);
          this->in(ChNameValue<ChFunctorArchiveIn>(bVal.name(), specFuncA));
      }

        /// Operator to allow easy serialization as   myarchive << mydata;

      template<class T>
      ChArchiveIn& operator>>(ChNameValue<T> bVal) {
          this->in(bVal);
          return (*this);
      }
      
      int VersionRead() {
          if (use_versions) {
              int mver;
              this->in(ChNameValue<int>("version",mver));
              return mver;
          }
          return 99999;
      }

  protected:

};






 




}  // END_OF_NAMESPACE____

#endif
