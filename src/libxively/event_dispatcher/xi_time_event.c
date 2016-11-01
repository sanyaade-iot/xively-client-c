/* Copyright (c) 2003-2015, LogMeIn, Inc. All rights reserved.
 * This is part of Xively C library. */

#include "xi_time_event.h"

/**
 * @brief This part of the file implements time event functionality. This
 * implementation assumes that the element type is always the xi_time_event_t.
 *
 * It uses the vector as a container type. The vector stores all teh time event related
 * values.
 *
 */

/**
 * @brief xi_swap_time_events
 *
 * Swaps two vector xi_time_event_t elements pointed by fi and li indexes with each other.
 * It updates element's positions value to the new ones.
 *
 * @param vector
 * @param fi
 * @param li
 */
static void xi_swap_time_events( xi_vector_t* vector,
                                 xi_vector_index_type_t fi,
                                 xi_vector_index_type_t li )
{
    /* PRE-CONDITIONS */
    assert( NULL != vector );
    assert( fi < vector->elem_no );
    assert( li < vector->elem_no );

    xi_time_event_t* fte = ( xi_time_event_t* )vector->array[fi].selector_t.ptr_value;
    xi_time_event_t* lte = ( xi_time_event_t* )vector->array[li].selector_t.ptr_value;

    xi_vector_swap_elems( vector, fi, li );

    fte->position = li;
    lte->position = fi;
}

xi_vector_index_type_t
xi_time_event_bubble_and_sort_down( xi_vector_t* vector, xi_vector_index_type_t index )
{
    /* prepare the tmp variables that will keep the indexes */
    xi_vector_index_type_t elem_index        = index;
    xi_vector_index_type_t elem_to_cmp_index = elem_index - 1;

    /* let's bubble up the element to the proper position */
    while ( elem_to_cmp_index >= 0 )
    {
        const xi_time_event_t* elem_to_cmp =
            ( xi_time_event_t* )vector->array[elem_to_cmp_index].selector_t.ptr_value;
        const xi_time_event_t* elem_to_bubble =
            ( xi_time_event_t* )vector->array[elem_index].selector_t.ptr_value;

        if ( elem_to_cmp->time_of_execution > elem_to_bubble->time_of_execution )
        {
            /* if the elements are not in the right order lets swap them */
            xi_swap_time_events( vector, elem_to_cmp_index, elem_index );

            /* update the positions */
            elem_index = elem_to_cmp_index;
            elem_to_cmp_index -= 1;
        }
        else
        {
            /* thanks to the container invariant on the order of the elemements we can
             * break at this moment */
            break;
        }
    }

    return elem_index;
}

void
xi_time_event_move_to_the_end( xi_vector_t* vector, xi_vector_index_type_t index )
{
    assert( vector->elem_no > 0 );
    assert( index < vector->elem_no );
    assert( index >= 0 );

    const xi_vector_index_type_t last_elem_index = vector->elem_no - 1;

    xi_vector_index_type_t elem_to_swap_with_index = index + 1;

    while ( index < last_elem_index )
    {
        xi_swap_time_events( vector, index, elem_to_swap_with_index );

        /* once swapped update the indexes */
        index += 1;
        elem_to_swap_with_index += 1;
    }
}

/**
 * @brief xi_insert_time_event
 *
 * This function uses an idea of bubble sort in order to put the given time_event onto the
 * proper place inside the vector.
 *
 * @param vector
 * @param time_event
 */
static const xi_vector_elem_t*
xi_insert_time_event( xi_vector_t* vector, xi_time_event_t* time_event )
{
    xi_state_t local_state = XI_STATE_OK;

    /* add the element to the end of the vector */
    const xi_vector_elem_t* inserted_element = xi_vector_push(
        vector, XI_VEC_CONST_VALUE_PARAM( XI_VEC_VALUE_PTR( time_event ) ) );

    XI_CHECK_MEMORY( inserted_element, local_state );

    /* update the time event new position */
    time_event->position = vector->elem_no - 1;

    xi_time_event_bubble_and_sort_down( vector, vector->elem_no - 1 );

    return inserted_element;

err_handling:
    return NULL;
}

static void xi_time_event_clean_time_event( xi_time_event_t* time_event )
{
    if ( NULL != time_event->time_event_handle )
    {
        time_event->time_event_handle->position = NULL;
    }
}

xi_state_t xi_time_event_add( xi_vector_t* vector,
                              xi_time_event_t* time_event,
                              xi_time_event_handle_t* ret_time_event_handle )
{
    /* PRE-CONDITIONS */
    assert( NULL != vector );
    assert( NULL != time_event );
    assert(
        ( NULL != ret_time_event_handle && NULL == ret_time_event_handle->position ) ||
        ( NULL == ret_time_event_handle ) );

    xi_state_t out_state = XI_STATE_OK;

    /* call the insert at function it will place the new element at the proper place */
    const xi_vector_elem_t* elem = xi_insert_time_event( vector, time_event );

    /* if there is a problem with the memory go to err_handling */
    XI_CHECK_MEMORY( elem, out_state );

    /* extract the element */
    xi_time_event_t* added_time_event = ( xi_time_event_t* )elem->selector_t.ptr_value;

    /* sanity checks */
    assert( added_time_event == time_event );

    if ( NULL != ret_time_event_handle )
    {
        /* update the return value with the pointer to the position in the vector */
        ret_time_event_handle->position = &added_time_event->position;

        /* set the time event handle pointer for further sanity checks and cleaning */
        added_time_event->time_event_handle = ret_time_event_handle;
    }

    /* exit with out_state value */
    return out_state;

err_handling:
    return out_state;
}

xi_time_event_t* xi_time_event_get_top( xi_vector_t* vector )
{
    /* PRE-CONDITIONS */
    assert( NULL != vector );

    if ( 0 == vector->elem_no )
    {
        return NULL;
    }

    xi_time_event_t* top_one = ( xi_time_event_t* )vector->array[0].selector_t.ptr_value;

    /* the trick is to swap the first element with the last one and to proceed bottom
     * direction fix of heap datastructure */

    if ( vector->elem_no > 1 )
    {
        xi_time_event_move_to_the_end( vector, 0 );
        xi_vector_del( vector, vector->elem_no - 1 );
    }
    else
    {
        xi_vector_del( vector, vector->elem_no - 1 );
    }

    xi_time_event_clean_time_event( top_one );

    return top_one;
}

xi_time_event_t* xi_time_event_peek_top( xi_vector_t* vector )
{
    /* PRE-CONDITIONS */
    assert( NULL != vector );

    if ( 0 == vector->elem_no )
    {
        return NULL;
    }

    xi_time_event_t* top_one = ( xi_time_event_t* )vector->array[0].selector_t.ptr_value;

    return top_one;
}

xi_state_t xi_time_event_restart( xi_vector_t* vector,
                                  xi_time_event_handle_t* time_event_handle,
                                  xi_time_t new_time )
{
    /* PRE-CONDITIONS */
    assert( NULL != vector );
    assert( NULL != time_event_handle );

    /* the element can be found with O(1) complexity cause we've been updating each
     * element's position during every operation that could've broken it */

    xi_vector_index_type_t index = *time_event_handle->position;

    /* check for the correctness of the time_event_handle */
    if ( index >= vector->elem_no || index < 0 )
    {
        return XI_ELEMENT_NOT_FOUND;
    }

    /* let's update the key of this element */
    xi_time_event_t* time_event =
        ( xi_time_event_t* )vector->array[index].selector_t.ptr_value;

    /* sanity check on the time handle */
    assert( time_event->time_event_handle == time_event_handle );

    time_event->time_of_execution = new_time;

    /* now we have to restore the order in the vector */
    /* this is very lame implementation but it works and doesn't require additional code */
    xi_time_event_move_to_the_end( vector, index );
    xi_time_event_bubble_and_sort_down( vector, vector->elem_no - 1 );

    return XI_STATE_OK;
}

xi_state_t xi_time_event_cancel( xi_vector_t* vector,
                                 xi_time_event_handle_t* time_event_handle,
                                 xi_time_event_t** cancelled_time_event )
{
    /* PRE-CONDITIONS */
    assert( NULL != vector );
    assert( NULL != time_event_handle );
    assert( NULL != time_event_handle->position );
    assert( NULL != cancelled_time_event );

    /* the element we would like to remove should be at position described by the
     * time_event_handle */

    xi_vector_index_type_t index = *time_event_handle->position;

    if ( index >= vector->elem_no || index < 0 )
    {
        return XI_ELEMENT_NOT_FOUND;
    }

    /* if it's somwhere else than the end, let's swap it with the last element */
    if ( index < vector->elem_no - 1 )
    {
        xi_time_event_move_to_the_end( vector, index );
    }

    /* let's update the return parameter */
    *cancelled_time_event =
        ( xi_time_event_t* )vector->array[vector->elem_no - 1].selector_t.ptr_value;

    /* now we can remove that element from the vector */
    xi_vector_del( vector, vector->elem_no - 1 );

    xi_time_event_clean_time_event( *cancelled_time_event );

    return XI_STATE_OK;
}

/* local helper function used to release the memory required for time event */
static void xi_time_event_destructor( union xi_vector_selector_u* selector, void* arg )
{
    XI_UNUSED( arg );

    xi_time_event_t* time_event = ( xi_time_event_t* )selector->ptr_value;
    time_event->position        = XI_TIME_EVENT_POSITION_INVALID;
    XI_SAFE_FREE( time_event );
}

void xi_time_event_destroy( xi_vector_t* vector )
{
    xi_vector_for_each( vector, &xi_time_event_destructor, NULL, 0 );
}
