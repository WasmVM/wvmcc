# Generated — clause 6.2 (Concepts). Tests assert ISO C17; partial/deferred (and
# many catalog-supported) rows are EXPECTED TO FAIL — that is the conformance signal.
# Regenerated from workflow metadata; do not hand-edit.

add_standard_run_test(lang-6.2.1-01 ${CMAKE_CURRENT_SOURCE_DIR}/language/6.2_concepts/6.2.1_goto_function_scope.c supported)
add_standard_run_test(lang-6.2.1-02 ${CMAKE_CURRENT_SOURCE_DIR}/language/6.2_concepts/6.2.1_block_scope_hides_outer.c supported)
add_standard_compile_fail(lang-6.2.1-03 ${CMAKE_CURRENT_SOURCE_DIR}/language/6.2_concepts/6.2.1_block_scope_not_visible_after.c supported)
add_standard_run_test(lang-6.2.1-04 ${CMAKE_CURRENT_SOURCE_DIR}/language/6.2_concepts/6.2.1_prototype_scope_param_names.c supported)
add_standard_run_test(lang-6.2.1-05 ${CMAKE_CURRENT_SOURCE_DIR}/language/6.2_concepts/6.2.1_tag_in_scope_self_reference.c supported)
add_standard_run_test(lang-6.2.1-06 ${CMAKE_CURRENT_SOURCE_DIR}/language/6.2_concepts/6.2.1_enum_constant_usable_after.c supported)
add_standard_run_test(lang-6.2.2-02 ${CMAKE_CURRENT_SOURCE_DIR}/language/6.2_concepts/6.2.2_extern_keeps_prior_linkage.c supported)
add_standard_run_test(lang-6.2.2-04 ${CMAKE_CURRENT_SOURCE_DIR}/language/6.2_concepts/6.2.2_function_external_linkage.c supported)
add_standard_run_test(lang-6.2.2-05 ${CMAKE_CURRENT_SOURCE_DIR}/language/6.2_concepts/6.2.2_block_scope_no_linkage.c supported)
add_standard_run_test(lang-6.2.3-01 ${CMAKE_CURRENT_SOURCE_DIR}/language/6.2_concepts/6.2.3_tag_and_ordinary.c supported)
add_standard_run_test(lang-6.2.3-02 ${CMAKE_CURRENT_SOURCE_DIR}/language/6.2_concepts/6.2.3_label_and_ordinary.c supported)
add_standard_run_test(lang-6.2.3-03 ${CMAKE_CURRENT_SOURCE_DIR}/language/6.2_concepts/6.2.3_member_namespace.c supported)
add_standard_run_test(lang-6.2.4-01 ${CMAKE_CURRENT_SOURCE_DIR}/language/6.2_concepts/6.2.4_static_storage_duration.c supported)
add_standard_run_test(lang-6.2.4-02 ${CMAKE_CURRENT_SOURCE_DIR}/language/6.2_concepts/6.2.4_automatic_storage_per_recursion.c supported)
add_standard_run_test(lang-6.2.4-03 ${CMAKE_CURRENT_SOURCE_DIR}/language/6.2_concepts/6.2.4_static_block_scope_persists.c supported)
add_standard_static_assert(lang-6.2.5-01 ${CMAKE_CURRENT_SOURCE_DIR}/language/6.2_concepts/6.2.5_bool_stores_zero_one.c supported)
add_standard_static_assert(lang-6.2.5-02 ${CMAKE_CURRENT_SOURCE_DIR}/language/6.2_concepts/6.2.5_char_holds_basic_set.c supported)
add_standard_static_assert(lang-6.2.5-03 ${CMAKE_CURRENT_SOURCE_DIR}/language/6.2_concepts/6.2.5_char_is_signed.c supported)
add_standard_static_assert(lang-6.2.5-04 ${CMAKE_CURRENT_SOURCE_DIR}/language/6.2_concepts/6.2.5_signed_char_storage_int_range.c supported)
add_standard_static_assert(lang-6.2.5-05 ${CMAKE_CURRENT_SOURCE_DIR}/language/6.2_concepts/6.2.5_standard_signed_integer_sizes.c supported)
add_standard_run_test(lang-6.2.5-07 ${CMAKE_CURRENT_SOURCE_DIR}/language/6.2_concepts/6.2.5_unsigned_wraps_modulo.c supported)
add_standard_static_assert(lang-6.2.5-08 ${CMAKE_CURRENT_SOURCE_DIR}/language/6.2_concepts/6.2.5_real_floating_value_sets.c partial)
add_standard_static_assert(lang-6.2.5-10 ${CMAKE_CURRENT_SOURCE_DIR}/language/6.2_concepts/6.2.5_three_char_types.c supported)
add_standard_static_assert(lang-6.2.5-11 ${CMAKE_CURRENT_SOURCE_DIR}/language/6.2_concepts/6.2.5_enum_constants_and_type.c supported)
add_standard_compile_fail(lang-6.2.5-12 ${CMAKE_CURRENT_SOURCE_DIR}/language/6.2_concepts/6.2.5_sizeof_void_rejected.c supported)
add_standard_run_test(lang-6.2.5-13 ${CMAKE_CURRENT_SOURCE_DIR}/language/6.2_concepts/6.2.5_derived_types.c supported)
add_standard_static_assert(lang-6.2.5-15 ${CMAKE_CURRENT_SOURCE_DIR}/language/6.2_concepts/6.2.5_pointer_representation.c supported)
add_standard_run_test(lang-6.2.6.1-01 ${CMAKE_CURRENT_SOURCE_DIR}/language/6.2_concepts/6.2.6.1_object_rep_memcpy.c supported)
add_standard_static_assert(lang-6.2.6.1-02 ${CMAKE_CURRENT_SOURCE_DIR}/language/6.2_concepts/6.2.6.1_unsigned_pure_binary.c supported)
add_standard_run_test(lang-6.2.6.1-03 ${CMAKE_CURRENT_SOURCE_DIR}/language/6.2_concepts/6.2.6.1_byte_order.c supported)
add_standard_static_assert(lang-6.2.6.2-01 ${CMAKE_CURRENT_SOURCE_DIR}/language/6.2_concepts/6.2.6.2_integer_rep_no_padding.c supported)
add_standard_run_test(lang-6.2.6.2-02 ${CMAKE_CURRENT_SOURCE_DIR}/language/6.2_concepts/6.2.6.2_twos_complement.c supported)
add_standard_static_assert(lang-6.2.6.2-05 ${CMAKE_CURRENT_SOURCE_DIR}/language/6.2_concepts/6.2.6.2_all_zero_is_zero.c supported)
add_standard_static_assert(lang-6.2.6.2-06 ${CMAKE_CURRENT_SOURCE_DIR}/language/6.2_concepts/6.2.6.2_width_precision.c supported)
# Cross-TU object external linkage (#84): two-TU link+run. The file-scope
# object `shared` (external linkage) is defined in the main TU and referenced
# via `extern` by the aux TU; both denote the same object.
add_standard_run_test2(lang-6.2.2-03 ${CMAKE_CURRENT_SOURCE_DIR}/language/6.2_concepts/6.2.2_object_external_linkage.c ${CMAKE_CURRENT_SOURCE_DIR}/language/6.2_concepts/6.2.2_object_external_linkage_aux.c supported)
add_standard_compile_fail(lang-6.2.7-02 ${CMAKE_CURRENT_SOURCE_DIR}/language/6.2_concepts/6.2.7_incompatible_redeclaration.c supported)
add_standard_run_test(lang-6.2.7-04 ${CMAKE_CURRENT_SOURCE_DIR}/language/6.2_concepts/6.2.7_composite_type.c supported)
add_standard_static_assert(lang-6.2.8-01 ${CMAKE_CURRENT_SOURCE_DIR}/language/6.2_concepts/6.2.8_object_type_alignment.c supported)
add_standard_static_assert(lang-6.2.8-02 ${CMAKE_CURRENT_SOURCE_DIR}/language/6.2_concepts/6.2.8_fundamental_alignment_max_align_t.c supported)
add_standard_static_assert(lang-6.2.8-03 ${CMAKE_CURRENT_SOURCE_DIR}/language/6.2_concepts/6.2.8_alignas_stricter_alignment.c supported)
add_standard_static_assert(lang-6.2.8-04 ${CMAKE_CURRENT_SOURCE_DIR}/language/6.2_concepts/6.2.8_extended_overalignment.c supported)

# Multi-object user-TU linking now works (#84): lang-6.2.2-03 (object external
# linkage) is registered above and passes. The remaining two cross-TU rows link
# fine but are blocked on pre-existing *codegen* gaps (not linking):
#   lang-6.2.2-01  — address-of a file-scope `static` object miscompiles
#                    (`func[1]: empty validate value stack` at -c on its own TU).
#   lang-6.2.7-01  — struct-by-value parameter/return miscompiles even single-TU
#                    (mk()/sum() return wrong values; the by-value arg case
#                    produces an invalid module). Register once codegen is fixed.
