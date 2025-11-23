//
// Created by Christian Redl on 21.11.25.
//

#include "server/ldap/ldap_utils.h"

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <ldap.h>
#include <string>

bool ldap_authenticate(const std::string& user, std::string& password) {

   if (user.empty()) {
      std::cerr << "User cannot be empty\n";
      return false;
   }

   if (password.empty()) {
      std::cerr << "Password cannot be empty\n";
      return false;
   }

   // LDAP config
   // anonymous bind with user and pw empty
   const char *ldapUri = "ldap://ldap.technikum-wien.at:389";
   const int ldapVersion = LDAP_VERSION3;

   std::string dn = "uid=" + user + ",ou=people,dc=technikum-wien,dc=at";

   // setup LDAP connection
   LDAP *ldapHandle;
   int rc = ldap_initialize(&ldapHandle, ldapUri);
   if (rc != LDAP_SUCCESS)
   {
      std::cerr << "ldap_initialize(): " << ldap_err2string(rc) << std::endl;
      return false;
   }
   printf("connected to LDAP server %s\n", ldapUri);

   // set version options
   rc = ldap_set_option(
       ldapHandle,
       LDAP_OPT_PROTOCOL_VERSION, // OPTION
       &ldapVersion);             // IN-Value
   if (rc != LDAP_OPT_SUCCESS)
   {
      std::cerr << "ldap_set_optio(PROTOCOL_VERSION): " << ldap_err2string(rc) << std::endl;
      ldap_unbind_ext_s(ldapHandle, nullptr, nullptr);
      return false;
   }

   // Start TLS
   rc = ldap_start_tls_s(
       ldapHandle,
       NULL,
       NULL);
   if (rc != LDAP_SUCCESS)
   {
      std::cerr << "ldap_start_tls_s(): " << ldap_err2string(rc) << std::endl;
      ldap_unbind_ext_s(ldapHandle, NULL, NULL);
      return false;
   }

   // bind credentials

   BerValue bindCredentials;
   bindCredentials.bv_val = (char *)password.c_str();
   bindCredentials.bv_len = password.size();
   BerValue *servercredp = nullptr; // server's credentials
   rc = ldap_sasl_bind_s(
       ldapHandle,
       dn.c_str(),
       LDAP_SASL_SIMPLE,
       &bindCredentials,
       NULL,
       NULL,
       &servercredp);

   if (rc != LDAP_SUCCESS)
   {
      fprintf(stderr, "LDAP bind error: %s\n", ldap_err2string(rc));
      ldap_unbind_ext_s(ldapHandle, NULL, NULL);
      return false;
   }

   std::fill(password.begin(), password.end(), '\0');

   ldap_unbind_ext_s(ldapHandle, NULL, NULL);

   return rc == LDAP_SUCCESS;
}
