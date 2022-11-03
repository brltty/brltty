.. |product name| replace:: Raspbian Canute Controller
.. |document subject| replace:: |product name| Software

======================
The |document subject|
======================

The |document subject| is Copyright ©
2022
by Bristol Braille Technology CIC.

.. contents::

Installation and Upgrading
==========================

An archive of the |document subject| can be downloaded in the following formats:

  =====  ==============================
  bzip2  `<canute-controller.tar.bz2>`_
  gzip   `<canute-controller.tar.gz>`_
  xz     `<canute-controller.tar.xz>`_
  =====  ==============================

To install it on your **Raspbian** system,
download the `<canute-install>`_ script and run it as **root**.

After it has been installed, you can upgrade it by
running the `canute-upgrade`_ script as **root**.

Commands
========

Session Management
------------------

.. include:: canute-startx.help.rst
.. include:: canute-screen.help.rst
.. include:: canute-ttysize.help.rst

System Administration
---------------------

.. include:: canute-upgrade.help.rst
.. include:: canute-status.help.rst
.. include:: canute-start.help.rst
.. include:: canute-stop.help.rst
.. include:: canute-enable.help.rst
.. include:: canute-disable.help.rst

Set/Inspect BrlAPI Parameters
-----------------------------

.. include:: canute-apitool.help.rst
.. include:: canute-parameter.help.rst
.. include:: canute-cbt.help.rst
.. include:: canute-lbt.help.rst
.. include:: canute-client.help.rst

Miscellaneous Commands
----------------------

.. include:: weather-current.help.rst

