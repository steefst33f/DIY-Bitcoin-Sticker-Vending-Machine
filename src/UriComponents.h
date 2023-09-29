/// Source: Solution 2: https://9to5answer.com/easy-way-to-parse-a-url-in-c-cross-platform
#include <string>
#include <algorithm>

#pragma once

#ifndef UriComponents_h
  #define UriComponents_h

   struct UriComponents {
      public:
      std::string queryString, path, protocol, host, port;

      static UriComponents Parse(const std::string &uri) {
         UriComponents result;

         typedef std::string::const_iterator iterator_t;

         if (uri.length() == 0)
            return result;

         iterator_t uriEnd = uri.end();

         // get query start
         iterator_t queryStart = std::find(uri.begin(), uriEnd, '?');

         // protocol
         iterator_t protocolStart = uri.begin();
         iterator_t protocolEnd = std::find(protocolStart, uriEnd, ':');            //"://");

         if (protocolEnd != uriEnd) {
            std::string prot = &*(protocolEnd);
            if ((prot.length() > 3) && (prot.substr(0, 3) == "://")) {
               result.protocol = std::string(protocolStart, protocolEnd);
               protocolEnd += 3;   //      ://
            } else {
               protocolEnd = uri.begin();  // no protocol
            }
         } else {
            protocolEnd = uri.begin();  // no protocol
         }

         // host
         iterator_t hostStart = protocolEnd;
         iterator_t pathStart = std::find(hostStart, uriEnd, '/');  // get pathStart

         // check for port
         iterator_t hostEnd = std::find(
            protocolEnd, 
            (pathStart != uriEnd) ? pathStart : queryStart,
            ':'
         );  

         result.host = std::string(hostStart, hostEnd);

         // port
         if ((hostEnd != uriEnd) && ((&*(hostEnd))[0] == ':')) { //if port
            hostEnd++;
            iterator_t portEnd = (pathStart != uriEnd) ? pathStart : queryStart;
            result.port = std::string(hostEnd, portEnd);
         }

         // path
         if (pathStart != uriEnd) {
            result.path = std::string(pathStart, queryStart);
         }
         
         // query
         if (queryStart != uriEnd) {
            result.queryString = std::string(queryStart, uri.end());
         }
         return result;
      }   // Parse
   };  // uri
#endif
