#ifndef ODFS_PROCESS_PATH
#define ODFS_PROCESS_PATH
#include "limits.h"
#include "odfs.h"
#include <stdio.h>
#include <stdlib.h>

/******************************************************************************/
/* ver 0.0.3                                                                  */
/* / - means AND                                                              */
/* # - means OR                                                               */
/*                                                                            */
/* /odfs/[plugin_name:][tag_name=]{tag_value| }                               */
/*                                                                            */
/* [tag_name=]{tag_value| } --> tag_string                                    */
/*                                                                            */
/* tag_string goes to RUN_STACK interface which checks                        */
/* in plugin tag_string and returns id                                        */
/*                                                                            */
/* i.e.:                                                                      */
/* ls /odfs/movies:actor=Cruse/movies:actor=Travolta                          */
/* process path gets string                                                   */
/*  - /movies:actor=Cruse/movies:actor=Travolta                               */
/*  - sends (movies, actor=Cruse) to RUN_STACK                                */
/*  - stores id list                                                          */
/*  - sends (movies, actor=Travolta) to RUN_STACK                             */
/*  - stores id list                                                          */
/*  - creates one id_list from two results, and returns it                    */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/* ver 0.0.2                                                                  */
/* default path :                                                             */
/* /odfs/id=123                                                               */
/* function will get id=123 and should return 123                             */
/*                                                                            */
/* /odfs/id=123#id=124     ( # means AND )                                    */
/* function will get id=123#id=124 and return 123, 124                        */
/*                                                                            */
/* /odfs/id=123##id=124    ( ## means OR )                                    */
/* function will get id=123#id=124 and return 123, 124                        */
/*                                                                            */
/* next :                                                                     */
/* - check different than 'id' tags, and convert to id                        */
/* - before return check if id exists                                         */
/* - implement logic of : #, ##                                               */
/*                                                                            */
/******************************************************************************/

/*
int process_path (const char *path, odfs_id_list *list) {
   int result = 0;
   //int id_counter = 0;
   //long int val;
   char *token, *path_buf, *tofree;
   struct odfs_tag_query  query;

   printf ("process_path path=%s\n",path);
   
   

   list->number_of_ids=0;

   if (strncmp(path, hello_path,strlen(hello_path)) == 0) {
      _get_newfile_id(path,list);
      return result;
   }

   query.plugin_name="name";
   query.list=list;
   query.context=CTX_PLUGIN_PP;

   tofree = path_buf = strdup(path);
   while ((token = strsep(&path_buf, "/")) != NULL) {
      //if(token + 1 != path_buf) (name_plugin->get)(token,list);
      if(token + 1 != path_buf) {
         query.tag_string=token;   
         odfs_run_plugin_stack(tp_stack, &query);
      } 
   }
   free(tofree);
   //id_counter = list->number_of_ids;
//   while (*path) { 
//      if (isdigit(*path)) {
//         val = strtol(path, &path, 10); // Read a number, ...
//         list->id_list[id_counter] = (unsigned int) val ;
//         //printf ("process_path add id=%u to list\n",(unsigned int ) val);
//         id_counter++;
//         //printf("%ld\n", val); 
//      } else { 
//         path++;
//      }
//   }
//   list->number_of_ids = id_counter ;
//

   return result;
}
*/

// stara wersja : //int process_path (const char *path, odfs_id_list_head *list)
/**
* \brief This is a process_path function      
*
* \param *path is const char pointer ( this is example only )
*
* This is a function which is a function, that is now official it is function.
*/

#define ODFS_NULL_ID UINT_MAX
#define DEBUG 1

static int process_path (const char *path, odfs_id_list_node *list, odfs_tag_stack * stack);
static int display_odfs_id_list(odfs_id_list_node *list);

static int init_id_list(odfs_id_list_node ** list){

   (*list) = malloc( sizeof (odfs_id_list_node) );
   (*list)->next = NULL;
   (*list)->odfs_id = ODFS_NULL_ID;
   return 0;
}


/**
* \fc search for odfs_id value in list
*
* (1) - value already exists, (0) - value doesn't exists, (-1) - error
*/
static int search_for_id_in_list(unsigned int val, odfs_id_list_node *list)
{
   if(DEBUG) { fprintf(stderr, "%s:%d debug: search_for_id_in_list val =%d\n",__FILE__,__LINE__,val); }
   int result = 0;

   odfs_id_list_node *current_node;

   current_node = list;

   while(current_node->next != NULL)
   {
      if( (current_node->odfs_id == val) )
      {
         //value already exists
         result = 1;
         return result;
      }
      current_node = current_node->next;
   }

   //last node
   if( (current_node->odfs_id == val) )
   {
      //value already exists
      result = 1;
      return result;
   }

   return result;

}// - end int search_for_id_in_list(unsigned int val, odfs_id_list *list)


/**
* \fc append single odfs_id value to list, avoid adding duplicate values
*
* opis, opis, description, description
*
*/
static int add_id_to_list(unsigned int val, odfs_id_list_node *list)
{
   int result = 0;
   odfs_id_list_node *current_node;
   odfs_id_list_node *test=list;
   if(DEBUG) { fprintf(stderr, "%s:%d debug: add_id_to_list val=%d\n",__FILE__,__LINE__,val); }

   //sprawdzenie czy element juz nie istnieje
   result = search_for_id_in_list(val, list);
  
   if( result == 0 )
   {
       //rewind to last
       while(list->next != NULL)
       {
          list = list->next;
       }

      if (list->odfs_id != ODFS_NULL_ID) {
          list->next = malloc( sizeof(odfs_id_list_node) );
          //brak sprawdzenia powodzenia malloc
          current_node = list->next;
          current_node->odfs_id = val;
          current_node->next = NULL ;
      } else {
          list->odfs_id = val;
          list->next = NULL ; //TODO to chyba nie potrzebne
      }
   }
   else if( result < 0 )
   {
      result = (-1); // error
      return result;   
   }

   return result;   //0  - succes
                    //1  - succes, id already existed in list
                    //-1 - error
}// end - int add_id_to_list(unsigned int val, odfs_id_list *list)


/**
* \brief append odfs_id_list to second argument (also odfs_id_list)
*
*/
static int append_odfs_id_list(odfs_id_list_node *new_elements_list, odfs_id_list_node *target_list)
{
   int result = 0;
   unsigned int new_odfs_id = 0;

   if(DEBUG) { fprintf(stderr, "%s:%d debug: append_odfs_id_list\n",__FILE__,__LINE__); }
   if(new_elements_list == NULL)
   {
      if(DEBUG) { fprintf(stderr, "%s:%d debug: empty list, no id added to list \n",__FILE__,__LINE__); }
      result = 0;
      return result;
   }
      
   if(DEBUG) { fprintf(stderr, "%s:%d debug: append_odfs_id_list\n",__FILE__,__LINE__); }
   if(new_elements_list->odfs_id < 1)
   {
      printf("error: list->odfs_id less than one \n");
      result = (-1);
      return result;
   }

   if(DEBUG) { fprintf(stderr, "%s:%d debug: append_odfs_id_list\n",__FILE__,__LINE__); }

   //rewind to last
   while(target_list->next != NULL)
   {
      target_list = target_list->next;
   }

   if(DEBUG) { fprintf(stderr, "%s:%d debug: append_odfs_id_list\n",__FILE__,__LINE__); }

   while(new_elements_list->next != NULL)
   {
      new_odfs_id = new_elements_list->odfs_id;
      if (new_odfs_id != ODFS_NULL_ID ) result = add_id_to_list(new_odfs_id, target_list);
      if(result < 0)
      {
         printf("%s:%d error: in append_odfs_id_list when add_id_to_list\n",__FILE__,__LINE__);
         result = (-1);
         return result;
      }
      new_elements_list = new_elements_list->next ;
   if(DEBUG) { fprintf(stderr, "%s:%d debug: append_odfs_id_list\n",__FILE__,__LINE__); }
   }

   //append last element
   new_odfs_id = new_elements_list->odfs_id;
   result = 0;
   if (new_odfs_id != ODFS_NULL_ID ) result = add_id_to_list(new_odfs_id, target_list);
   if(result < 0)
   {
      result = (-1);
      return result;
   }

   if(DEBUG) { fprintf(stderr, "%s:%d debug: append_odfs_id_list\n",__FILE__,__LINE__); }
   
   return result;

}// end - int append_odfs_id_list(odfs_id_list *new_elements_list, odfs_id_list *target_list)


/**
* \fc int clear_odfs_id_list(odfs_id_list *list)
*
* \brief frees memory and elements of list, leaves first
*
* \warning przydaloby sie uzywanie argumentu mowiacego o dlugosci 
* \warning listy, na wypadek gdyby sie NULL zgubil
*
*/
static int clear_odfs_id_list(odfs_id_list_node *list)
{
   if(DEBUG) { fprintf(stderr, "%s:%d debug: clear_odfs_id_list\n",__FILE__,__LINE__); }
   int result = 0;

   odfs_id_list_node *current_node;
   odfs_id_list_node *next_node;

   if(DEBUG) { fprintf(stderr, "%s:%d debug: clear_odfs_id_list \n",__FILE__,__LINE__); }

   if(list->odfs_id <= 0)
   {
      printf("error: list->odfs_id less than one \n");
      result = (-1);
      return result;
   }

   current_node = list;

   while(current_node->next != NULL)
   {
      next_node = current_node->next;
      if(DEBUG){ fprintf(stderr, "%s:%d debug: clear_odfs_id_list: free next element \n",__FILE__,__LINE__); }
      free(current_node);
      current_node = next_node;
   } 
   if(DEBUG){ fprintf(stderr, "%s:%d debug: clear_odfs_id_list: free last element \n",__FILE__,__LINE__); }
   free(current_node);

   list->next = NULL;
   list->odfs_id = 0;

   return result;
}// end - int clear_odfs_id_list(odfs_id_list *list)

static int count_odfs_id_list(odfs_id_list_node *list)
{
   if(DEBUG) { printf("%s:%d debug: count_odfs_id_list\n",__FILE__,__LINE__); }
   int count = 0;

   odfs_id_list_node *current_node;

   if(DEBUG)
   {
      printf("%s:%d debug: count_odfs_id_list \n",__FILE__,__LINE__);
   }

   current_node = list;

   while(current_node)
   {
      if (current_node->odfs_id == ODFS_NULL_ID) {
        break;
      } 
      count++;
      current_node = current_node->next;
   } 
   list->odfs_id = 0;

   return count;
}

/**
* \brief for debuging purposes, prints all odfs_id_list elements
*
*/
static int display_odfs_id_list(odfs_id_list_node *list)
{
   int result = 0;
   int elements_counter = 0;

   odfs_id_list_node *next_node;

   printf("display_odfs_id_list :\n");
   /* check for errors in list head */
   if( list == NULL )
   {
      printf("\terror in odfs_id_list  is NULL\n");
      return (-1);
   }
   next_node=list; 
   while (next_node != NULL) {
      elements_counter++;
      printf("\tnode : %d\n", elements_counter);
      printf("\t\tnode value: %ud\n", next_node->odfs_id);
      printf("\t\tpointer to next element : %p\n", next_node->next);
      next_node = next_node->next;
   }
   return result;
}// end - int display_odfs_id_list(odfs_id_list *list)


static int process_path (const char *path, odfs_id_list_node *list, odfs_tag_stack * stack)
{
   int patch_parts_counter=0; /* counts plugin_name+tag_string in path */
   char *token1, *token2, *path_buf1, *path_buf2, *tofree1, *tofree2;
   char *plugin_name=NULL, *tag_string=NULL;

   int result = 0;
   int semicolon_counter;

   odfs_tag_query query_struct;

   //lista wewnetrzna zawierajaca id zwracane z RUN_STACK
   odfs_id_list_node *list_internal;
   init_id_list(&list_internal);

   if(DEBUG) { fprintf(stderr, "%s:%d debug: process_path path=%s\n",__FILE__,__LINE__,path); }
   
   tofree1 = path_buf1 = strdup(path);
   while( (token1 = strsep(&path_buf1, "/")) != NULL )
   {
      if(DEBUG) { fprintf(stderr, "%s:%d debug: token1 value = %s\n",__FILE__,__LINE__,token1); }
      if(DEBUG) { fprintf(stderr, "%s:%d debug: path_buf1 value = %s\n",__FILE__,__LINE__,path_buf1); }
      semicolon_counter=0;
      if(token1 + 1 != path_buf1) // (+1) bo separator na poczatku
      {
         // wyszukanie ewentualnego ':' w token dzielacego na plugin_name i tag_string
         tofree2=path_buf2 = strdup(token1);
         while( (token2 = strsep(&path_buf2, ":")) != NULL )
         {
            if(DEBUG) { fprintf(stderr, "%s:%d debug: token2 value = %s\n",__FILE__,__LINE__,token2); }
            if(DEBUG) { fprintf(stderr, "%s:%d debug: path_buf2 value = %s\n",__FILE__,__LINE__,path_buf2); }

            if(token2 + 1 != path_buf2)
            {
               if(semicolon_counter > 1)
               {
                  //w tokenie wystapily wiecej niz jeden znak ':' 
                  result = -1;
                  if(DEBUG) { fprintf(stderr, "%s:%d debug: process_path result value =%d\n",__FILE__,__LINE__,result); }
                  return result;
               }

               if(semicolon_counter==1)
               {
                  tag_string = token2;
                  semicolon_counter++;
               }
   
               if(semicolon_counter==0)
               {
                  plugin_name = token2;
                  tag_string = token2; //przypadek kiedy nie ma ':'
                  semicolon_counter++;
               }
            }
         
         }// while((token2 = strsep(&path_buf2, ":")) != NULL)
         
         //wypelnienie struktury
         if(semicolon_counter==1) //nie bylo ':' nie ma czesci plugin_name
         {
            plugin_name = NULL;
         }

         if(DEBUG)
         {
            fprintf(stderr, "%s:%d debug: plugin_name: %s\n",__FILE__,__LINE__,plugin_name);
            fprintf(stderr, "%s:%d debug: tag_string: %s\n",__FILE__,__LINE__,tag_string);
         }
         query_struct.plugin_name = plugin_name;
         query_struct.tag_string = tag_string;
         query_struct.plugin_return_list = list_internal;
         query_struct.context = CTX_PLUGIN_PP;

         if(DEBUG)
         {
            fprintf(stderr, "%s:%d debug: query_struct\n",__FILE__,__LINE__);
            fprintf(stderr, "%s:%d debug:    query_struct.plugin_name: %s\n",__FILE__,__LINE__,query_struct.plugin_name);
            fprintf(stderr, "%s:%d debug:    query_struct.tag_string: %s\n",__FILE__,__LINE__,query_struct.tag_string);
         }
      
         //wywolanie RUN_STACK i otrzymanie listy id
         //TODO nie wiem jak to wywolac 
         //(name_plugin->get)(query_struct,list_internal);
         //

	 odfs_run_plugin_stack(stack, &query_struct);
         fprintf(stderr, "%s:%d debug:   odfs_run_plugin_stack\n",__FILE__,__LINE__);

         //dodaj zwrocone id do listy zbiorczej
         append_odfs_id_list(list_internal,list);
         fprintf(stderr, "%s:%d debug:   append_odfs_id_list\n",__FILE__,__LINE__);


         //clear_odfs_id_list(list_internal);
         //printf("%s:%d debug:   clear_odfs_id_list\n",__FILE__,__LINE__);

         //free must be last command in this loop
         free(tofree2);
      }
      patch_parts_counter++;

   }// while ((token1 = strsep(&path_buf1, "/")) != NULL)
   free(tofree1);
   
   //free(list_internal);

   if(DEBUG) { fprintf(stderr, "%s:%d debug: process_path result value =%d\n",__FILE__,__LINE__,result); }
   return result;
}


#endif

