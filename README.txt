This module provides a form for the Pay module which will POST data to the securetrading payment processer, where CC information is entered.

The module works by caching the Drupal form object that, before replacing the action with the url of the Payment Processor.

When the response comes back from the Payment Processor, the cached Drupal form object is rebuilt and submitted.

This should allow other Drupal events dependent upon the form being submitted to occur (eg webform submission).

This code is currently alpha-quality, it works for our use case, and has had some internal testing, but has not been tested in the wild as yet.

It may be that some changes to the Pay module could make this module more robust.
