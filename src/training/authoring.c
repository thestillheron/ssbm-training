#ifdef TRAINING_BUILD
#ifdef AUTHORING_BUILD

/// Present only in the training build with authoring tools: proves the
/// AUTHORING_BUILD macro reaches training code and never the player build.
/// Nothing references it, so it is forced active to survive linking.
#pragma force_active on
const char training_authoring_marker[] = "AUTHORING_BUILD_MARKER";
#pragma force_active reset

#endif
#endif
