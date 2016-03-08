/***********************************************************************
*
* Copyright (c) 2015-2016 Barbara Geller
* Copyright (c) 2015-2016 Ansel Sermersheim
* All rights reserved.
*
* This file is part of libCsSignal
*
* libCsSignal is free software: you can redistribute it and/or 
* modify it under the terms of the GNU Lesser General Public License
* version 2.1 as published by the Free Software Foundation.
*
* libCsSignal is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with CopperSpice.  If not, see 
* <http://www.gnu.org/licenses/>.
*
***********************************************************************/

#ifndef LIB_CS_INTERNAL_H
#define LIB_CS_INTERNAL_H

#include <memory>
#include <tuple>

namespace CsSignal {

class SlotBase;

namespace Internal {

// 1
template<class T, class U, class = void>
class cs_check_connect_args
   : public cs_check_connect_args<T, decltype(&U::operator())>
{
};


// 2 slot is a func ptr, first signal and first slot parameters match
template<class T, class ...ArgsX, class ...ArgsY>
class cs_check_connect_args<void (*)(T, ArgsX...), void (*)(T, ArgsY...)>
   : public cs_check_connect_args<void (*) (ArgsX...), void (*) (ArgsY...)>
{
};

//  slot is a func ptr, slot has no parameters
template<class ...ArgsX>
class cs_check_connect_args<void (*)(ArgsX...), void (*)()>
   : public std::integral_constant<bool, true>
{
};

//  slot is a func ptr, signal has the same number of parms as the slot, types mismatch
template<class ...ArgsX, class ...ArgsY>
class cs_check_connect_args < void (*)(ArgsX...), void (*)(ArgsY...),
   typename std::enable_if < sizeof...(ArgsX) == sizeof...(ArgsY) &&
   ! std::is_same<std::tuple<ArgsX...>, std::tuple<ArgsY...>>::value >::type >
         : public std::integral_constant<bool, false>
{
};

//  slot is a func ptr, signal has fewer number of parms than the slot
template<class ...ArgsX, class ...ArgsY>
class cs_check_connect_args < void (*)(ArgsX...), void (*)(ArgsY...),
   typename std::enable_if<sizeof...(ArgsX) < sizeof...(ArgsY)>::type >
   : public std::integral_constant<bool, false>
{
};


// 3 slot is a method ptr
template<class T, class...ArgsX, class...ArgsY>
class cs_check_connect_args<void (*)(ArgsX...), void (T::*)(ArgsY...) >
   : public cs_check_connect_args<void (*)(ArgsX...), void (*) (ArgsY...)>
{
};

//  slot is a const method ptr
template<class T, class...ArgsX, class...ArgsY>
class cs_check_connect_args<void (*)(ArgsX...), void (T::*)(ArgsY...) const>
   : public cs_check_connect_args<void (*)(ArgsX...), void (*) (ArgsY...)>
{
};


// compile time tests
template<class Sender, class SignalClass>
void cs_testConnect_SenderSignal()
{
   static_assert( std::is_base_of<SignalClass, Sender>::value,
                  "Signal is not defined in the sender class");
}

template<class Slot_LambdaType, class ...SignalArgs>
void cs_testConnect_SignalSlotArgs_1()
{
   static_assert( cs_check_connect_args<void (*)(SignalArgs...), Slot_LambdaType>::value,
                  "Incompatible signal/slot arguments");
}

template<class SlotClass, class Receiver>
void cs_testConnect_ReceiverSlot()
{
   static_assert( std::is_base_of<SlotClass, Receiver>::value,
                  "Slot is not defined in the receiver class");
}

template<class Signal_ArgType, class Slot_ArgType>
void cs_testConnect_SignalSlotArgs_2()
{
   static_assert( cs_check_connect_args<Signal_ArgType, Slot_ArgType>::value,
                  "Incompatible signal/slot arguments");
}

// available in C++14 
template <class T, class... Args>
std::unique_ptr<T> make_unique(Args &&... args) {
   return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

// marker for a function which returns a void
class CSVoidReturn
{
};

// ** index sequence (available in C++14)
// ** generate list of integers corresponding to the number of data types in a parameter pack
template<size_t...Ks>
class Index_Sequence
{
};

template<size_t S, class K1, class K2>
class Concat_Sequence;

template<size_t S, size_t ...M, size_t ...N>
class Concat_Sequence<S, Index_Sequence<M...>, Index_Sequence<N...>>
{
 public:
   using type = Index_Sequence < M... , (N + S)... >;
};

template<size_t N>
class Make_Index_Sequence
{
 public:
   using type = typename Concat_Sequence < (N / 2), typename Make_Index_Sequence < (N / 2) >::type,
         typename Make_Index_Sequence < (N - (N / 2)) >::type >::type;
};

template<>
class Make_Index_Sequence<0>
{
 public:
   using type = Index_Sequence<>;
};

template<>
class Make_Index_Sequence<1>
{
 public:
   using type = Index_Sequence<0>;
};

template<class ...Ts>
class Index_Sequence_For
{
 public:
   using type = typename Make_Index_Sequence<sizeof ...(Ts)>::type;
};


broom -> START HERE TO OPTIMIZE


// ** unpack_function   (1)
// ** uses Index_Sequence Class to unpack a tuple into arguments to a function

template<typename ...FunctionArgTypes, typename FunctionReturn, typename ...TupleTypes, size_t ...Ks>
FunctionReturn cs_unpack_function_args_internal(FunctionReturn (*functionPtr)(FunctionArgTypes...),
      const std::tuple<TupleTypes...> &data, Index_Sequence<Ks...>)
{   
   return functionPtr(std::get<Ks>(data)...);
}

// (api) function pointer unpack tuple
template<typename ...FunctionArgTypes, typename FunctionReturn, typename ...TupleTypes>
FunctionReturn cs_unpack_function_args(FunctionReturn (*functionPtr)(FunctionArgTypes...),
                                       const std::tuple<TupleTypes...> &data)
{
   return cs_unpack_function_args_internal(functionPtr, data, typename Index_Sequence_For<TupleTypes...>::type {} );
}

// specialization when FunctionReturn as type void, force to CSVoidReturn
template<typename ...FunctionArgTypes, typename ...TupleTypes>
CSVoidReturn cs_unpack_function_args(void (*functionPtr)(FunctionArgTypes...), const std::tuple<TupleTypes...> &data)
{
   cs_unpack_function_args_internal(functionPtr, data, typename Index_Sequence_For<TupleTypes...>::type {} );
   return CSVoidReturn {};
}


// ** unpack_function   (2)
// ** uses Index_Sequence Class to unpack a tuple into arguments to a method pointer

template<typename MethodClass, class MethodReturn, typename ...MethodArgTypes, typename ...TupleTypes, size_t ...Ks>
MethodReturn cs_unpack_method_args_internal(MethodClass *obj, MethodReturn (MethodClass::*methodPtr)(MethodArgTypes...),
                  const std::tuple<TupleTypes...> &data, Index_Sequence<Ks...> dummy)
{  
   return (obj->*methodPtr)(std::get<Ks>(data)...);
}

// ** const method pointer
template<typename MethodClass, class MethodReturn, typename ...MethodArgTypes, typename ...TupleTypes, size_t ...Ks>
MethodReturn cs_unpack_method_args_internal(const MethodClass *obj,
                  MethodReturn (MethodClass::*methodPtr)(MethodArgTypes...) const,
                  const std::tuple<TupleTypes...> &data, Index_Sequence<Ks...> dummy)
{
   return (obj->*methodPtr)(std::get<Ks>(data)...);
}

// (api) method pointer unpack tuple
template<typename MethodClass, class MethodReturn, typename ...MethodArgTypes, typename ...TupleTypes>
MethodReturn cs_unpack_method_args(MethodClass *obj, MethodReturn (MethodClass::*methodPtr)(MethodArgTypes...),
                  const std::tuple<TupleTypes...> &data)
{
   return cs_unpack_method_args_internal(obj, methodPtr, data, typename Index_Sequence_For<TupleTypes...>::type {} );
}

// specialization when MethodReturn has type void, force to CSVoidReturn
template<typename MethodClass, typename ...MethodArgTypes, typename ...TupleTypes>
CSVoidReturn cs_unpack_method_args(MethodClass *obj, void (MethodClass::*methodPtr)(MethodArgTypes...),
                  const std::tuple<TupleTypes...> &data)
{
   cs_unpack_method_args_internal(obj, methodPtr, data, typename Index_Sequence_For<TupleTypes...>::type {} );
   return CSVoidReturn {};
}

// (api) method pointer unpack tuple   (const method pointer)
template<typename MethodClass, class MethodReturn, typename ...MethodArgTypes, typename ...TupleTypes>
MethodReturn cs_unpack_method_args(const MethodClass *obj,
                  MethodReturn (MethodClass::*methodPtr)(MethodArgTypes...) const,
                  const std::tuple<TupleTypes...> &data)
{
   return cs_unpack_method_args_internal(obj, methodPtr, data, typename Index_Sequence_For<TupleTypes...>::type {} );
}

// specialization when MethodReturn as type void, force to CSVoidReturn  (const method pointer)
template<typename MethodClass, typename ...MethodArgTypes, typename ...TupleTypes>
CSVoidReturn cs_unpack_method_args(const MethodClass *obj, void (MethodClass::*methodPtr)(MethodArgTypes...) const,
                  const std::tuple<TupleTypes...> &data)
{
   cs_unpack_method_args_internal(obj, methodPtr, data, typename Index_Sequence_For<TupleTypes...>::type {} );
   return CSVoidReturn {};
}



template <class T1, class T2>
class prePend
{
   // required dummy class to utilze a specialization
};

template <class T, class ...Ts>
class prePend<T, std::tuple<Ts...>>
{
 public:
   using type = typename std::tuple<T, Ts...>;
};

template <class T1>
class strip
{
   public:
      // contains nothing
      using type = typename std::tuple<>;
};

template <class T1, class T2, class ...Ts>
class strip<std::tuple<T1, T2, Ts...>>
{
   public:
      using type = typename prePend<T1, typename strip<std::tuple<T2, Ts...> >::type>::type;      
};

template <class ...Ts>
class removeLast
{
   public:
      using type = typename strip< std::tuple<Ts...> >::type;     
};


// **
template<unsigned int ...Vs>
class intValues
{
 public:
   using type = intValues<Vs...>;
};

template<unsigned int Max, unsigned int ...Vs>
class makeIntValues : public makeIntValues < Max - 1, Max - 1, Vs... >
{
};

template<unsigned int...Vs>
class makeIntValues<0, Vs...> : public intValues<Vs...>
{
};

template<unsigned int ...Vs, class ...Ts>
typename removeLast<Ts...>::type newFunc(intValues<Vs...>, std::tuple<Ts...> tupleValue)
{
   return std::forward_as_tuple(std::get<Vs>(tupleValue)...);
}

template<class ...Ts>
typename removeLast<Ts...>::type funcRemove(std::tuple<Ts...> tupleValue )
{
   return newFunc(makeIntValues < sizeof...(Ts) - 1 > (), tupleValue);
}

template<class Last, class ...Ts>
std::tuple<Ts...> funcRemove(std::tuple<Ts..., Last> tupleValue, Ts...Vs )
{
   return std::forward_as_tuple(Vs...);
}


// ** store slot data in tuple
class TeaCupAbstract
{
   public:
      virtual ~TeaCupAbstract() {}      
};

// 1
template<class ...Ts>
class TeaCup : public TeaCup< typename removeLast<Ts...>::type >
{
   public:
      template<class T>
      explicit TeaCup(T lambda);
      
      std::tuple<Ts...> getData() const;
   
   private:
      std::function<std::tuple<Ts...> ()> m_lambda;
};

template<class ...Ts>
template<class T>
TeaCup<Ts...>::TeaCup(T lambda)
   : TeaCup< typename removeLast<Ts...>::type >( [this]() { return funcRemove(m_lambda()); } ), 
     m_lambda(lambda) 
{
}

template<class ...Ts>
std::tuple<Ts...> TeaCup<Ts...>::getData() const
{
   return m_lambda();
}

// 2  specialization, empty data
template<>
class TeaCup<>: public TeaCupAbstract
{
 public:
   template<class T>
   explicit TeaCup(T lambda);

   std::tuple<> getData() const;
};

template<class T>
TeaCup<>::TeaCup(T lambda)
{
}

inline std::tuple<> TeaCup<>::getData() const
{
   // empty tuple
   return std::tuple<> {};
}


// 3  specialization, tuple
template<class ...Ts>
class TeaCup< std::tuple<Ts...> >: public TeaCup<Ts...>
{
 public:
   template<class T>
   explicit TeaCup(T lambda);
};

template<class ...Ts>
template<class T>
TeaCup<std::tuple<Ts...>>::TeaCup(T lambda) 
   : TeaCup<Ts...>(lambda)
{
}


// ** next two functions use Index_Sequence Class to convert a tuple to
template<class R, class T, size_t ...Ks>
R convert_tuple_internal(T &data, Index_Sequence<Ks...> dummy)
{
   return R {std::get<Ks>(data)...};
}

template<class R, class ...Ts>
R convert_tuple(std::tuple<Ts...> &data)
{
   return convert_tuple_internal<R> (data, typename Index_Sequence_For<Ts...>::type {} );
}


// ** TeaCup Class used to store data for signal activation
template<class ...Ts>
class TeaCup_Data: public TeaCup<Ts...>
{
   public:
      TeaCup_Data(bool needs_Copying, Ts...);
      std::tuple<Ts...> getData() const;     
   
   private:
      std::shared_ptr< std::tuple<typename std::remove_reference<Ts>::type...> > m_copyOfData;
      std::tuple<Ts...> m_data;
};

template<class ...Ts>
TeaCup_Data<Ts...>::TeaCup_Data(bool needs_Copying, Ts...Vs)
   : TeaCup<Ts...>( [this]() { return m_data; } ),
     m_copyOfData(needs_Copying ? new std::tuple<typename std::remove_reference<Ts>::type...> (Vs...) : nullptr ),
     m_data(needs_Copying ? convert_tuple<std::tuple<Ts...>> (*m_copyOfData) : std::tuple<Ts...> (Vs...) )
{
}

template<class ...Ts>
std::tuple<Ts...> TeaCup_Data<Ts...>::getData() const
{
   return m_data;
}


// ** store method pointer for signals and slots
class BentoAbstract
{
   public:
      virtual ~BentoAbstract() {}
   
      virtual bool operator ==(const BentoAbstract &right) const = 0;
      bool operator !=(const BentoAbstract &right) const;

      virtual void invoke(SlotBase *receiver, const TeaCupAbstract *dataPack) const = 0; 
      virtual std::unique_ptr<BentoAbstract> clone() const = 0;  
};

inline bool BentoAbstract::operator !=(const BentoAbstract &right) const
{
   return ! (*this == right);
}


template<class T>
class Bento : public BentoAbstract
{
   public:
      Bento(T ptr);

      virtual std::unique_ptr<BentoAbstract> clone() const override;  
      virtual bool operator ==(const BentoAbstract &right) const override;

      virtual void invoke(SlotBase *receiver, const TeaCupAbstract *dataPack) const;
       
      template<class MethodReturn, class ...MethodArgs>
      void invoke_internal(const TeaCupAbstract *dataPack, MethodReturn (T::*methodPtr)(MethodArgs...) const) const;

      template<class MethodReturn, class ...MethodArgs>
      void invoke_internal(const TeaCupAbstract *dataPack, MethodReturn (T::*methodPtr)(MethodArgs...)) const;

      T m_lambda;
};

template<class FunctionReturn, class ...FunctionArgs>
class Bento<FunctionReturn (*)(FunctionArgs...)> : public BentoAbstract
{
   public:
      Bento(FunctionReturn (*ptr)(FunctionArgs...));

      virtual std::unique_ptr<BentoAbstract> clone() const override;
      virtual bool operator ==(const BentoAbstract &right) const;
      virtual void invoke(SlotBase *receiver, const TeaCupAbstract *dataPack) const;
 
      FunctionReturn (*m_methodPtr)(FunctionArgs...);
};

template<class MethodClass, class MethodReturn, class...MethodArgs>
class Bento<MethodReturn(MethodClass::*)(MethodArgs...)>: public BentoAbstract
{
   public:
      Bento(MethodReturn(MethodClass::*ptr)(MethodArgs...) );

      virtual std::unique_ptr<BentoAbstract> clone() const override;
      virtual bool operator ==(const BentoAbstract &right) const;
      virtual void invoke(SlotBase *receiver, const TeaCupAbstract *dataPack) const;
  
      MethodReturn(MethodClass::*m_methodPtr)(MethodArgs...);
};

template<class MethodClass, class MethodReturn, class...MethodArgs>
class Bento<MethodReturn(MethodClass::*)(MethodArgs...) const>: public BentoAbstract
{
   // specialization, pointer to const method

   public:
      Bento(MethodReturn(MethodClass::*ptr)(MethodArgs...) const);

      virtual std::unique_ptr<BentoAbstract> clone() const override;
      virtual bool operator ==(const BentoAbstract &right) const;
      virtual void invoke(SlotBase *receiver, const TeaCupAbstract *dataPack) const;
  
      MethodReturn(MethodClass::*m_methodPtr)(MethodArgs...) const;
};


// (1) lambda
template<class T>
Bento<T>::Bento(T lambda)
   : m_lambda(lambda)
{
}

template<class T>
std::unique_ptr<BentoAbstract> Bento<T>::clone() const
{
   return make_unique<Bento<T>>(*this);   
}

template<class T>
bool Bento<T>::operator ==(const BentoAbstract &) const
{
   // can not compare two lambdas
   return false;
}

template<class T>
void Bento<T>::invoke(SlotBase *receiver, const TeaCupAbstract *dataPack) const
{
   // T must be a class, will be a compiler error otherwise
   auto methodPtr = &T::operator();

   this->invoke_internal(dataPack, methodPtr);
}

template<class T>
template<class MethodReturn, class ...MethodArgs>
void Bento<T>::invoke_internal(const TeaCupAbstract *dataPack, MethodReturn (T::*methodPtr)(MethodArgs...) const) const
{
   // handles non-mutable, captures variables are const

   // dynamic cast will return a valid ptr if the slot has equal or less parameters
   // retrieve ptr to teaCup object, which contains the data
   const TeaCup<MethodArgs...> *teaCup = dynamic_cast<const TeaCup<MethodArgs...> *>(dataPack);

   if (teaCup) {
      // expand arguments
      std::tuple<MethodArgs...> args = teaCup->getData();

      // unpacks the tuple, then calls the method or slot
      cs_unpack_method_args(&m_lambda, methodPtr, args);
   }
}

template<class T>
template<class MethodReturn, class ...MethodArgs>
void Bento<T>::invoke_internal(const TeaCupAbstract *dataPack, MethodReturn (T::*methodPtr)(MethodArgs...)) const
{
   // handles mutable, captures variables are non-const

   // dynamic cast will return a valid ptr if the slot has equal or less parameters
   // retrieve ptr to teaCup object, which contains the data
   const TeaCup<MethodArgs...> *teaCup = dynamic_cast<const TeaCup<MethodArgs...> *>(dataPack);

   if (teaCup) {
      // expand arguments
      std::tuple<MethodArgs...> args = teaCup->getData();

      // prep m_lambda
      auto object = const_cast<typename std::remove_const<T>::type *>(&m_lambda);

      // unpacks the tuple, then calls the method or slot
      cs_unpack_method_args(object, methodPtr, args);
   }
}


// (2) specialization, function pointer
template<class FunctionReturn, class ...FunctionArgs>
Bento<FunctionReturn (*)(FunctionArgs...)>::Bento(FunctionReturn (*ptr)(FunctionArgs...)) :
   m_methodPtr(ptr)
{
}

template<class FunctionReturn, class ...FunctionArgs>
std::unique_ptr<BentoAbstract> Bento<FunctionReturn (*)(FunctionArgs...)>::clone() const
{
   return make_unique<Bento<FunctionReturn (*)(FunctionArgs...)>>(*this);  
}

template<class FunctionReturn, class ...FunctionArgs>
bool Bento<FunctionReturn (*)(FunctionArgs...)>::operator ==(const BentoAbstract &right) const
{
   bool retval = false;

   const Bento<FunctionReturn (*)(FunctionArgs...)> *temp;
   temp = dynamic_cast<const Bento <FunctionReturn (*)(FunctionArgs...)> *> (&right);

   if (temp) {
      retval = (this->m_methodPtr == temp->m_methodPtr);
   }

   return retval;
}

template<class FunctionReturn, class ...FunctionArgs>
void Bento<FunctionReturn (*)(FunctionArgs...)>::invoke(SlotBase *receiver, const TeaCupAbstract *dataPack) const
{
   // no need to verify receiver since it is not used

   // dynamic cast will return a valid ptr if the slot has equal or less parameters
   // retrieve ptr to teaCup object, which contains the data
   const TeaCup<FunctionArgs...> *teaCup = dynamic_cast<const TeaCup<FunctionArgs...> *>(dataPack);

   if (teaCup) {
      // expand arguments
      std::tuple<FunctionArgs...> args = teaCup->getData();
   
      // unpack the tuple, then call the method or slot
      cs_unpack_function_args(m_methodPtr, args);      
   }
}


// (3) specialization, method pointer
template<class MethodClass, class MethodReturn, class...MethodArgs>
Bento<MethodReturn(MethodClass::*)(MethodArgs...)>::Bento(MethodReturn(MethodClass::*ptr)(MethodArgs...)) :
   m_methodPtr(ptr)
{
}

template<class MethodClass, class MethodReturn, class...MethodArgs>
std::unique_ptr<BentoAbstract> Bento<MethodReturn(MethodClass::*)(MethodArgs...)>::clone() const
{
   return make_unique<Bento<MethodReturn(MethodClass::*)(MethodArgs...)>>(*this); 
}

template<class MethodClass, class MethodReturn, class...MethodArgs>
bool Bento<MethodReturn(MethodClass::*)(MethodArgs...)>::operator ==(const BentoAbstract &right) const
{
   bool retval = false;

   const Bento<MethodReturn(MethodClass::*)(MethodArgs...)> *temp;
   temp = dynamic_cast<const Bento <MethodReturn(MethodClass::*)(MethodArgs...)> *> (&right);

   if (temp) {
      retval = (this->m_methodPtr == temp->m_methodPtr);
   }

   return retval;
}

template<class MethodClass, class MethodReturn, class ...MethodArgs>
void Bento<MethodReturn(MethodClass::*)(MethodArgs...)>::invoke(SlotBase *receiver, const TeaCupAbstract *dataPack) const
{
   if (! receiver)  {
      return;
   }

   MethodClass *t_receiver = dynamic_cast<MethodClass *>(receiver);

   if (t_receiver) {
      // dynamic cast will return a valid ptr if the slot has equal or less parameters
      // retrieve ptr to teaCup object, which contains the data
      const TeaCup<MethodArgs...> *teaCup = dynamic_cast<const TeaCup<MethodArgs...> *>(dataPack);

      if (teaCup) {
         // expand arguments
         std::tuple<MethodArgs...> args = teaCup->getData();       

         // unpacks the tuple, then calls the method or slot
         cs_unpack_method_args(t_receiver, m_methodPtr, args);
      }
   }
}


// (4) specialization, pointer to const method
template<class MethodClass, class MethodReturn, class...MethodArgs>
Bento<MethodReturn(MethodClass::*)(MethodArgs...) const>::Bento(MethodReturn(MethodClass::*ptr)(MethodArgs...) const) :
   m_methodPtr(ptr)
{
}

template<class MethodClass, class MethodReturn, class...MethodArgs>
std::unique_ptr<BentoAbstract> Bento<MethodReturn(MethodClass::*)(MethodArgs...) const>::clone() const
{
   return make_unique<Bento<MethodReturn(MethodClass::*)(MethodArgs...) const>>(*this); 
}

template<class MethodClass, class MethodReturn, class...MethodArgs>
bool Bento<MethodReturn(MethodClass::*)(MethodArgs...) const>::operator ==(const BentoAbstract &right) const
{
   bool retval = false;

   const Bento<MethodReturn(MethodClass::*)(MethodArgs...) const> *temp;
   temp = dynamic_cast<const Bento <MethodReturn(MethodClass::*)(MethodArgs...) const> *> (&right);

   if (temp) {
      retval = (this->m_methodPtr == temp->m_methodPtr);
   }

   return retval;
}

template<class MethodClass, class MethodReturn, class ...MethodArgs>
void Bento<MethodReturn(MethodClass::*)(MethodArgs...) const>::invoke(SlotBase *receiver, const TeaCupAbstract *dataPack) const
{
   if (! receiver)  {
      return;
   }

   MethodClass *t_receiver = dynamic_cast<MethodClass *>(receiver);

   if (t_receiver) {
      // dynamic cast will return a valid ptr if the slot has equal or less parameters

      // retrieve ptr to teaCup object, which contains the data
      const TeaCup<MethodArgs...> *teaCup = dynamic_cast<const TeaCup<MethodArgs...> *>(dataPack);

      if (teaCup) {
         // expand arguments
         std::tuple<MethodArgs...> args = teaCup->getData();      

         // unpacks the tuple, then calls the method or slot
         cs_unpack_method_args(t_receiver, m_methodPtr, args);        
      }
   }
}

} }

#endif