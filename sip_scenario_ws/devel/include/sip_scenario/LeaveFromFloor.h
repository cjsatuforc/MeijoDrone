// Generated by gencpp from file sip_scenario/LeaveFromFloor.msg
// DO NOT EDIT!


#ifndef SIP_SCENARIO_MESSAGE_LEAVEFROMFLOOR_H
#define SIP_SCENARIO_MESSAGE_LEAVEFROMFLOOR_H

#include <ros/service_traits.h>


#include <sip_scenario/LeaveFromFloorRequest.h>
#include <sip_scenario/LeaveFromFloorResponse.h>


namespace sip_scenario
{

struct LeaveFromFloor
{

typedef LeaveFromFloorRequest Request;
typedef LeaveFromFloorResponse Response;
Request request;
Response response;

typedef Request RequestType;
typedef Response ResponseType;

}; // struct LeaveFromFloor
} // namespace sip_scenario


namespace ros
{
namespace service_traits
{


template<>
struct MD5Sum< ::sip_scenario::LeaveFromFloor > {
  static const char* value()
  {
    return "df4b6884ffadda4d31d3c1de4bbcf17e";
  }

  static const char* value(const ::sip_scenario::LeaveFromFloor&) { return value(); }
};

template<>
struct DataType< ::sip_scenario::LeaveFromFloor > {
  static const char* value()
  {
    return "sip_scenario/LeaveFromFloor";
  }

  static const char* value(const ::sip_scenario::LeaveFromFloor&) { return value(); }
};


// service_traits::MD5Sum< ::sip_scenario::LeaveFromFloorRequest> should match 
// service_traits::MD5Sum< ::sip_scenario::LeaveFromFloor > 
template<>
struct MD5Sum< ::sip_scenario::LeaveFromFloorRequest>
{
  static const char* value()
  {
    return MD5Sum< ::sip_scenario::LeaveFromFloor >::value();
  }
  static const char* value(const ::sip_scenario::LeaveFromFloorRequest&)
  {
    return value();
  }
};

// service_traits::DataType< ::sip_scenario::LeaveFromFloorRequest> should match 
// service_traits::DataType< ::sip_scenario::LeaveFromFloor > 
template<>
struct DataType< ::sip_scenario::LeaveFromFloorRequest>
{
  static const char* value()
  {
    return DataType< ::sip_scenario::LeaveFromFloor >::value();
  }
  static const char* value(const ::sip_scenario::LeaveFromFloorRequest&)
  {
    return value();
  }
};

// service_traits::MD5Sum< ::sip_scenario::LeaveFromFloorResponse> should match 
// service_traits::MD5Sum< ::sip_scenario::LeaveFromFloor > 
template<>
struct MD5Sum< ::sip_scenario::LeaveFromFloorResponse>
{
  static const char* value()
  {
    return MD5Sum< ::sip_scenario::LeaveFromFloor >::value();
  }
  static const char* value(const ::sip_scenario::LeaveFromFloorResponse&)
  {
    return value();
  }
};

// service_traits::DataType< ::sip_scenario::LeaveFromFloorResponse> should match 
// service_traits::DataType< ::sip_scenario::LeaveFromFloor > 
template<>
struct DataType< ::sip_scenario::LeaveFromFloorResponse>
{
  static const char* value()
  {
    return DataType< ::sip_scenario::LeaveFromFloor >::value();
  }
  static const char* value(const ::sip_scenario::LeaveFromFloorResponse&)
  {
    return value();
  }
};

} // namespace service_traits
} // namespace ros

#endif // SIP_SCENARIO_MESSAGE_LEAVEFROMFLOOR_H
