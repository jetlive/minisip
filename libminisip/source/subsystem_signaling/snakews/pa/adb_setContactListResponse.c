

        /**
         * adb_setContactListResponse.c
         *
         * This file was auto-generated from WSDL
         * by the Apache Axis2/C version: SNAPSHOT  Built on : Mar 10, 2008 (08:35:52 GMT+00:00)
         */

        #include "adb_setContactListResponse.h"
        
                /*
                 * This type was generated from the piece of schema that had
                 * name = setContactListResponse
                 * Namespace URI = http://esb.presenceagent.services.hdviper.psnc.pl/
                 * Namespace Prefix = ns1
                 */
           


        struct adb_setContactListResponse
        {
            
        };


       /************************* Private Function prototypes ********************************/
        


       /************************* Function Implmentations ********************************/
        adb_setContactListResponse_t* AXIS2_CALL
        adb_setContactListResponse_create(
            const axutil_env_t *env)
        {
            adb_setContactListResponse_t *_setContactListResponse = NULL;
            
            AXIS2_ENV_CHECK(env, NULL);

            _setContactListResponse = (adb_setContactListResponse_t *) AXIS2_MALLOC(env->
                allocator, sizeof(adb_setContactListResponse_t));

            if(NULL == _setContactListResponse)
            {
                AXIS2_ERROR_SET(env->error, AXIS2_ERROR_NO_MEMORY, AXIS2_FAILURE);
                return NULL;
            }

            memset(_setContactListResponse, 0, sizeof(adb_setContactListResponse_t));

            

            return _setContactListResponse;
        }

        axis2_status_t AXIS2_CALL
        adb_setContactListResponse_free (
                adb_setContactListResponse_t* _setContactListResponse,
                const axutil_env_t *env)
        {
            

            AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
            AXIS2_PARAM_CHECK(env->error, _setContactListResponse, AXIS2_FAILURE);

            

            if(_setContactListResponse)
            {
                AXIS2_FREE(env->allocator, _setContactListResponse);
                _setContactListResponse = NULL;
            }
            return AXIS2_SUCCESS;
        }


        

        axis2_status_t AXIS2_CALL
        adb_setContactListResponse_deserialize(
                adb_setContactListResponse_t* _setContactListResponse,
                const axutil_env_t *env,
                axiom_node_t **dp_parent,
                axis2_bool_t *dp_is_early_node_valid,
                axis2_bool_t dont_care_minoccurs)
        {
          axiom_node_t *parent = *dp_parent;
          
          axis2_status_t status = AXIS2_SUCCESS;
          
            axutil_qname_t *element_qname = NULL; 
            
            AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
            AXIS2_PARAM_CHECK(env->error, _setContactListResponse, AXIS2_FAILURE);

            
          return status;
       }

          axis2_bool_t AXIS2_CALL
          adb_setContactListResponse_is_particle()
          {
            
                 return AXIS2_FALSE;
              
          }


          void AXIS2_CALL
          adb_setContactListResponse_declare_parent_namespaces(
                    adb_setContactListResponse_t* _setContactListResponse,
                    const axutil_env_t *env, axiom_element_t *parent_element,
                    axutil_hash_t *namespaces, int *next_ns_index)
          {
            
                  /* Here this is an empty function, Nothing to declare */
                 
          }

        
        
        axiom_node_t* AXIS2_CALL
        adb_setContactListResponse_serialize(
                adb_setContactListResponse_t* _setContactListResponse,
                const axutil_env_t *env, axiom_node_t *parent, axiom_element_t *parent_element, int parent_tag_closed, axutil_hash_t *namespaces, int *next_ns_index)
        {
            
            
             axis2_char_t *string_to_stream;
            
         
         axiom_node_t *current_node = NULL;
         int tag_closed = 0;

         
         
                axiom_namespace_t *ns1 = NULL;

                axis2_char_t *qname_uri = NULL;
                axis2_char_t *qname_prefix = NULL;
                axis2_char_t *p_prefix = NULL;
                axis2_bool_t ns_already_defined;
            
            
               axiom_data_source_t *data_source = NULL;
               axutil_stream_t *stream = NULL;

            

            AXIS2_ENV_CHECK(env, NULL);
            AXIS2_PARAM_CHECK(env->error, _setContactListResponse, NULL);
            
            
                    current_node = parent;
                    data_source = (axiom_data_source_t *)axiom_node_get_data_element(current_node, env);
                    if (!data_source)
                        return NULL;
                    stream = axiom_data_source_get_stream(data_source, env); /* assume parent is of type data source */
                    if (!stream)
                        return NULL;
                  
            if(!parent_tag_closed)
            {
            
              string_to_stream = ">"; 
              axutil_stream_write(stream, env, string_to_stream, axutil_strlen(string_to_stream));
              tag_closed = 1;
            
            }
            

            return parent;
        }


        

