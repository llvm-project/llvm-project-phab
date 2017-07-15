============================
Release Notes 5.0 (upcoming)
============================

In Polly 5 the following important changes have been incorporated.

.. warning::

  These releaes notes are for the next release of Polly and describe
  the new features that have recently been committed to our development
  branch.

- Change ...

-----------------------------------
Robustness testing: AOSP and FFMPEG
-----------------------------------

Polly can now compile all of Android. While most of Android is not the primary
target of polyhedral data locality optimizations, Android provides us with a
large and diverse set of robustness tests.  Our new `nightly build bot
<http://lab.llvm.org:8011/builders/aosp-O3-polly-before-vectorizer-unprofitable>`_
ensures we do not regress.

Polly also successfully compiles `FFMPEG <http://fate.ffmpeg.org/>`_ and
obviously the `LLVM test suite
<http://lab.llvm.org:8011/console?category=polly>`_.

---------------------------------------------------------
C++ bindings for isl math library improve maintainability
---------------------------------------------------------

In the context of `Polly Labs <pollylabs.org>`_, a new set of C++ bindings was
developed for the isl math library. Thanks to the new isl C++ interface there
is no need for manual memory management any more and programming with integer
sets became easier in general.

Before::

    __isl_give isl_set *do_some_operations(__isl_take isl_set *s1, 
            __isl_take isl_set *s2, __isl_keep isl_set *s3, 
                    __isl_take isl_set *s4) {

      isl_set *s5 = isl_set_intersect(isl_set_copy(s1), s2);
      s5 = isl_set_subtract(s5, isl_set_union(isl_set_copy(s3), s4);

      if (isl_set_is_empty(s5)) {
        isl_set_free(s1);
        isl_set_free(s5);
        return isl_set_copy(s3);
      } else {
        isl_set_free(s5);
        return s1;
      }
    }

Today::

    isl::set do_some_operations(isl::set s1, isl::set s2, isl::set s3, 
                                                         isl::set s4) {
      isl::set s5 = s1.intersect(s2);
      s5 -= s3 + s4;

      if (s5.is_empty())
        return s3;
      else
        return s1;
    }

Besides this goal of increased readability we also introduce these new isl
bindings in a -- LLVM typical -- gradual way. Hence, we often will have 
functionality in isl or in Polly that still uses the old C interface. To 
interact with such functionality we need access to the raw pointers. Hence, 
we get a mix of C and C++ code, that could look as follows:

    isl::set do_some_operations(isl::set s1, isl::set s2, isl::set s3, 
                                                          isl::set s4) {
      isl::set s5 = s1.intersect(s2);

      s5 -= isl::manage(isl_set_union(s3.copy(); s4.release()));

      if (isl_set_is_empty(s5.get()))
        return s3;
      else
        return s1;
    }

Here we use isl::manage to convert a raw isl C pointer back to the C++ object,
and we use isl::set::copy() and isl::set::release() to obtain raw pointers from
s3 and s4, that represent a set identical to s3 and s4, which can be passed and
later be consumed by isl_set_union. For object s3 we need to use copy() to 
obtain a reference-counted copy of the s3 pointer, as the s3 object may 
possibly be used later. For s4 we can use release() which extracts the pointer
that backs s4 and invalidates s4. This is possible as s4 is not used later on.
Similarly, we use s5.get() to obtain a raw pointer that can be passed to 
isl_set_is_empty but that will not be consumed by isl_set_is_empty.


For reference:

    __isl_give *isl_set_union(__isl_take isl_set *s1, __isl_take *s2);
    int isl_set_is_empty(__isl_keep isl_set *s);
